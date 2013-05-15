/**
 * resultset.h
 *
 */

#ifndef _RESULTSET_
#define _RESULTSET_

#include "neo4j_fdw.h"
#include "stringlist.h"

/* row data */
typedef struct RowData {
	StringItemList* data;
	struct RowData* next;
} RowData;

/* Result Set */
typedef struct ResultSet {
	int column_num;
	StringItemList* columns;
	int rows;
	RowData* first;
	RowData* current;
	RowData* last;
} ResultSet;

extern ResultSet* createResultSet(StringItemList* sil);
extern RowData* createRowData(StringItemList* sil);
extern int addRowData(ResultSet* rs, RowData* rd);
extern void initResultSet(ResultSet* rs);
extern RowData* getFirstResultSet(ResultSet* rs);
extern RowData* getNextResultSet(ResultSet* rs);
extern char* getValueRowData(RowData* rd, int index);
extern void freeRowData(RowData* rd);
extern void freeResultSet(ResultSet* rs);
void printResultSet(ResultSet* rs);

#endif // _RESULTSET_
