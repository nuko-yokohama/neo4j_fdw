# contrib/neo4j_fdw/Makeneo4j

MODULE_big = neo4j_fdw
OBJS = json2resultset.o neo4j_accessor.o resultset.o stringlist.o neo4j_fdw.o

EXTENSION = neo4j_fdw
DATA = neo4j_fdw--1.0.sql

REGRESS = neo4j_fdw

EXTRA_CLEAN = sql/neo4j_fdw.sql expected/neo4j_fdw.out

SHLIB_LINK = -lcurl -ljson

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/neo4j_fdw
top_builddir = ../..
include $(top_builddir)/src/Makeneo4j.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
