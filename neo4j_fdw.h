/**
 * neo4j_fdw.h
 *
 */

#ifndef _NEO4J_FDW_
#define _NEO4J_FDW_

#ifdef UNIT_TEST
#define MALLOC malloc
#define FREE free
#else
#include <postgres.h>
#define MALLOC palloc
#define FREE pfree
#endif // UNIT_TEST

#endif // _NEO4J_FDW_

