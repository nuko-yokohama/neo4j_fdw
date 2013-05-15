/*-------------------------------------------------------------------------
 *
 * neo4j_fdw.c
 *		  foreign-data wrapper for Neo4j(Graph Database) 
 *
 *
 *-------------------------------------------------------------------------
 */
#include <sys/stat.h>
#include <unistd.h>

#include "postgres.h"
#include "funcapi.h"

#include "access/reloptions.h"
#include "catalog/pg_type.h"
#include "catalog/pg_foreign_table.h"
#include "commands/copy.h"
#include "commands/defrem.h"
#include "commands/explain.h"
#include "commands/vacuum.h"
#include "foreign/fdwapi.h"
#include "foreign/foreign.h"
#include "miscadmin.h"
#include "nodes/makefuncs.h"
#include "optimizer/cost.h"
#include "optimizer/pathnode.h"
#include "optimizer/planmain.h"
#include "optimizer/restrictinfo.h"
#include "utils/builtins.h"
#include "utils/memutils.h"
#include "utils/bytea.h"
#include "utils/elog.h"
#include "utils/rel.h"

/** fdw headers */
#include "neo4j_fdw.h"
#include "neo4j_accessor.h"
#include "stringlist.h"
#include "resultset.h"
#include "json2resultset.h"

PG_MODULE_MAGIC;

/*
 * Describes the valid options for objects that use this wrapper.
 */
struct FileFdwOption
{
	const char *optname;
	Oid			optcontext;		/* Oid of catalog in which option may appear */
};

/*
 * Valid options for file_fdw.
 * These options are based on the options for COPY FROM command.
 * But note that force_not_null is handled as a boolean option attached to
 * each column, not as a table option.
 *
 * Note: If you are adding new option for user mapping, you need to modify
 * fileGetOptions(), which currently doesn't bother to look at user mappings.
 */
static const struct FileFdwOption valid_options[] = {
	/* File options */
	{"filename", ForeignTableRelationId},

	/* Format options */
	/* oids option is not supported */
	{"format", ForeignTableRelationId},
	{"header", ForeignTableRelationId},
	{"delimiter", ForeignTableRelationId},
	{"quote", ForeignTableRelationId},
	{"escape", ForeignTableRelationId},
	{"null", ForeignTableRelationId},
	{"encoding", ForeignTableRelationId},
	{"force_not_null", AttributeRelationId},

	/*
	 * force_quote is not supported by file_fdw because it's for COPY TO.
	 */

	/* Sentinel */
	{NULL, InvalidOid}
};

/*
 * FDW-specific information for RelOptInfo.fdw_private.
 */
typedef struct neo4jFdwPlanState
{
	int nodes;		/* estimate of node in database */
	int edges;		/* estimate of edge in databese */
} neo4jFdwPlanState;

/*
 * FDW-specific information for ForeignScanState.fdw_state.
 */
typedef struct neo4jFdwExecutionState
{
	ResultSet	   *resultset;		/* 結果セット */
} neo4jFdwExecutionState;

/*
 * SQL functions
 */
extern Datum neo4j_fdw_handler(PG_FUNCTION_ARGS);
extern Datum neo4j_fdw_validator(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(neo4j_fdw_handler);
PG_FUNCTION_INFO_V1(neo4j_fdw_validator);

/*
 * FDW callback routines
 */
static void neo4jGetForeignRelSize(PlannerInfo *root,
					  RelOptInfo *baserel,
					  Oid foreigntableid);
static void neo4jGetForeignPaths(PlannerInfo *root,
					RelOptInfo *baserel,
					Oid foreigntableid);
static ForeignScan *neo4jGetForeignPlan(PlannerInfo *root,
				   RelOptInfo *baserel,
				   Oid foreigntableid,
				   ForeignPath *best_path,
				   List *tlist,
				   List *scan_clauses);
static void neo4jExplainForeignScan(ForeignScanState *node, ExplainState *es);
static void neo4jBeginForeignScan(ForeignScanState *node, int eflags);
static TupleTableSlot *neo4jIterateForeignScan(ForeignScanState *node);
static void neo4jReScanForeignScan(ForeignScanState *node);
static void neo4jEndForeignScan(ForeignScanState *node);
static bool neo4jAnalyzeForeignTable(Relation relation,
						AcquireSampleRowsFunc *func,
						BlockNumber *totalpages);

/*
 * Helper functions
 */
static void estimate_costs(PlannerInfo *root, RelOptInfo *baserel,
                           neo4jFdwPlanState *fdw_private,
                           Cost *startup_cost, Cost *total_cost);

static void
neo4jGetOptions(Oid foreigntableid,
			   char **url, char **query )
{
	ForeignTable *f_table;
	ForeignServer *f_server;
	ForeignDataWrapper *f_wrapper;
	List	   *options;
	ListCell   *lc;

	/*
	 * Extract options from FDW objects.  
	 */
	f_table = GetForeignTable(foreigntableid);
	f_server = GetForeignServer(f_table->serverid);
	f_wrapper = GetForeignDataWrapper(f_server->fdwid);

	options = NIL;
	options = list_concat(options, f_wrapper->options);
	options = list_concat(options, f_server->options);
	options = list_concat(options, f_table->options);

	/*
	 * Separate out the xmlname.
	 */
	*url = NULL;
	*query = NULL;
	foreach(lc, options)
	{
		DefElem    *def = (DefElem *) lfirst(lc);

		if (strcmp(def->defname, "url") == 0)
		{
			*url = defGetString(def);
		}
		if (strcmp(def->defname, "query") == 0)
		{
			*query = defGetString(def);
		}
	}

	/*
	 * The validator should have checked that a xmlname was included in the
	 * options, but check again, just in case.
	 */
	if (*url == NULL)
		elog(ERROR, "url is required for neo4j_fdw foreign server");
	if (*query == NULL)
		elog(ERROR, "query is required for neo4j_fdw foreign table");
}

/*
 * Foreign-data wrapper handler function: return a struct with pointers
 * to my callback routines.
 */
Datum
neo4j_fdw_handler(PG_FUNCTION_ARGS)
{
	FdwRoutine *fdwroutine = makeNode(FdwRoutine);

	fdwroutine->GetForeignRelSize = neo4jGetForeignRelSize;
	fdwroutine->GetForeignPaths = neo4jGetForeignPaths;
	fdwroutine->GetForeignPlan = neo4jGetForeignPlan;
	fdwroutine->ExplainForeignScan = neo4jExplainForeignScan;
	fdwroutine->BeginForeignScan = neo4jBeginForeignScan;
	fdwroutine->IterateForeignScan = neo4jIterateForeignScan;
	fdwroutine->ReScanForeignScan = neo4jReScanForeignScan;
	fdwroutine->EndForeignScan = neo4jEndForeignScan;
	fdwroutine->AnalyzeForeignTable = neo4jAnalyzeForeignTable;

	PG_RETURN_POINTER(fdwroutine);
}

/*
 * Validate the generic options given to a FOREIGN DATA WRAPPER, SERVER,
 * USER MAPPING or FOREIGN TABLE that uses neo4j_fdw.
 *
 * Raise an ERROR if the option or its value is considered invalid.
 */
Datum
neo4j_fdw_validator(PG_FUNCTION_ARGS)
{
	/* TODO */
	PG_RETURN_VOID();
}

/*
 * neo4jGetForeignRelSize
 *		Obtain relation size estimates for a foreign table
 */
static void
neo4jGetForeignRelSize(PlannerInfo *root,
					  RelOptInfo *baserel,
					  Oid foreigntableid)
{
	/* Estimate relation size */
	/* TODO */
}

/*
 * neo4jGetForeignPaths
 *		Create possible access paths for a scan on the foreign table
 *
 *		Currently we don't support any push-down feature, so there is only one
 *		possible access path, which simply returns all records in the order in
 *		the data file.
 */
static void
neo4jGetForeignPaths(PlannerInfo *root,
					RelOptInfo *baserel,
					Oid foreigntableid)
{
	/* referred to file_fdw. */
	neo4jFdwPlanState *fdw_private = (neo4jFdwPlanState *) baserel->fdw_private;
	Cost            startup_cost = 0;
	Cost            total_cost = 0;

        /* Estimate costs */
        estimate_costs(root, baserel, fdw_private,
                                   &startup_cost, &total_cost);

        /* Create a ForeignPath node and add it as only possible path */
        add_path(baserel, (Path *)
                         create_foreignscan_path(root, baserel,
                                                                         baserel->rows,
                                                                         startup_cost,
                                                                         total_cost,
                                                                         NIL,           /* no pathkeys */
                                                                         NULL,          /* no outer rel either */
                                                                         NIL));         /* no fdw_private data */

}

/*
 * neo4jGetForeignPlan
 *		Create a ForeignScan plan node for scanning the foreign table
 */
static ForeignScan *
neo4jGetForeignPlan(PlannerInfo *root,
				   RelOptInfo *baserel,
				   Oid foreigntableid,
				   ForeignPath *best_path,
				   List *tlist,
				   List *scan_clauses)
{
	/* ToDo: referred to file_fdw. */
        Index           scan_relid = baserel->relid;

        scan_clauses = extract_actual_clauses(scan_clauses, false);

        /* Create the ForeignScan node */
        return make_foreignscan(tlist,
                                                        scan_clauses,
                                                        scan_relid,
                                                        NIL,    /* no expressions to evaluate */
                                                        NIL);           /* no private state either */

}

/*
 * fileExplainForeignScan
 *		Produce extra output for EXPLAIN
 */
static void
neo4jExplainForeignScan(ForeignScanState *node, ExplainState *es)
{
	/* TODO  */
        ExplainPropertyText("Foreign wrapper", "neo4j", es);

        /* Suppress file size if we're not showing cost details */
        if (es->costs)
        {
            ExplainPropertyLong("Neo4j Costs", (long) 1000, es);
        }
}

/*
 * neo4jBeginForeignScan
 *		Initiate access to the file by creating CopyState
 */
static void
neo4jBeginForeignScan(ForeignScanState *node, int eflags)
{
	char	   *url;
	char	   *query;
	char	   *json;
	neo4jFdwExecutionState *festate;
	ResultSet* resultset;

	/*
	 * Do nothing in EXPLAIN (no ANALYZE) case.  node->fdw_state stays NULL.
	 */
	if (eflags & EXEC_FLAG_EXPLAIN_ONLY)
		return;

	/* Fetch options of foreign table */
	neo4jGetOptions(RelationGetRelid(node->ss.ss_currentRelation),
				   &url, &query);

	/* publish CyperQuery via REST-API for Neo4j and acquire JSON string */
	/* use curl library */
	json = neo4j_accessor(url, query);
	if (json == NULL) {
		elog(ERROR, "neo4j_accessor error, url=%s, query=%s", url, query);
	}
	elog(DEBUG1, "json=%s", json);

	/* TODO: generate a SELECT list from node. */
	// select_list = createSelectList(node);

	/* create ResultSet from json string. */
	/* use C-JSON library */
	resultset = json2resultset(json);
	if (resultset == NULL) {
		elog(ERROR, "json2resultset error, json=%s", json);
	}
 
	festate = (neo4jFdwExecutionState *) palloc(sizeof(neo4jFdwExecutionState));
	festate->resultset = resultset;

	node->fdw_state = (void *) festate;
}

/*
 * fileIterateForeignScan
 *		Read next record from the data file and store it into the
 *		ScanTupleSlot as a virtual tuple
 */
static TupleTableSlot *
neo4jIterateForeignScan(ForeignScanState *node)
{
	neo4jFdwExecutionState *festate = (neo4jFdwExecutionState *) node->fdw_state;
	TupleTableSlot *slot = node->ss.ss_ScanTupleSlot;
	HeapTuple	tuple;

	char** values;
	int column_num = festate->resultset->column_num;
	int i;
	RowData* rd;

	if ((rd = getNextResultSet(festate->resultset)) == NULL) {
		/* End of Data */
		ExecClearTuple(slot);
		return slot;
	}

	/* Set currecnt result for tuple */
	values = (char **) palloc(sizeof(char *) * column_num);
	for (i = 0; i < column_num; i++) {
		values[i] = getValueRowData(rd, i+1);
		if (values[i] == NULL) {
			elog(DEBUG1, "values[%d] is null.", i);
		}
	}

	/* set tuple */
        tuple = BuildTupleFromCStrings(TupleDescGetAttInMetadata(node->ss.ss_currentRelation->rd_att), values);
        ExecStoreTuple(tuple, slot, InvalidBuffer, false);


	return slot;

}

/*
 * neo4jReScanForeignScan
 *		Rescan table, possibly with new parameters
 */
static void
neo4jReScanForeignScan(ForeignScanState *node)
{
	neo4jFdwExecutionState *festate = (neo4jFdwExecutionState *) node->fdw_state;

	initResultSet(festate->resultset);
}

/*
 * neo4jEndForeignScan
 *		Finish scanning foreign table and dispose objects used for this scan
 */
static void
neo4jEndForeignScan(ForeignScanState *node)
{
	neo4jFdwExecutionState *festate = (neo4jFdwExecutionState *) node->fdw_state;

	/* if festate is NULL, we are in EXPLAIN; nothing to do */
        if (festate) {
		/* TODO: finish process */
        }
}

/*
 * neo4jAnalyzeForeignTable
 *		Test whether analyzing this foreign table is supported
 */
static bool
neo4jAnalyzeForeignTable(Relation relation,
						AcquireSampleRowsFunc *func,
						BlockNumber *totalpages)
{
	/* return false */
	return FALSE;
}

/*
 * Estimate costs of scanning a foreign table.
 *
 * Results are returned in *startup_cost and *total_cost.
 */
static void
estimate_costs(PlannerInfo *root, RelOptInfo *baserel,
			   neo4jFdwPlanState *fdw_private,
			   Cost *startup_cost, Cost *total_cost)
{
	/* NOP */
	/* TODO: calcurate cost... */
	return ;
}

PG_FUNCTION_INFO_V1(exec_cypher);

Datum exec_cypher(PG_FUNCTION_ARGS);

/**
 * connect neo4j database,
 * and execute Cypher Query.
 * return the result of the JSON form.
 *
 * add 2013-05-11
 */
Datum exec_cypher(PG_FUNCTION_ARGS) {
    text* arg1 = PG_GETARG_TEXT_PP(0);
    text* arg2 = PG_GETARG_TEXT_PP(1);
    char* url = text_to_cstring(arg1);
    char* query = text_to_cstring(arg2);
    char* result;

//    elog(NOTICE, "url=%s", url);
//    elog(NOTICE, "query=%s", query);
    result = neo4j_accessor(url, query);
//    elog(NOTICE, "result=%s", result);

    PG_RETURN_TEXT_P(cstring_to_text(result));
}

