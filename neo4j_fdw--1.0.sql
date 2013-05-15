/* neo4j_fdw--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION neo4j_fdw" to load this neo4j. \quit

CREATE FUNCTION neo4j_fdw_handler()
RETURNS fdw_handler
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT;

CREATE FUNCTION neo4j_fdw_validator(text[], oid)
RETURNS void
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT;

CREATE FOREIGN DATA WRAPPER neo4j_fdw
  HANDLER neo4j_fdw_handler
  VALIDATOR neo4j_fdw_validator;

-- Neo4j adhoc query execute function
CREATE FUNCTION exec_cypher(text, text)
RETURNS json
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT;

