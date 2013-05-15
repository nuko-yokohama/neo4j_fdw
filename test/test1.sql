CREATE EXTENSION neo4j_fdw;

CREATE SERVER foo
    FOREIGN DATA WRAPPER neo4j_fdw
    OPTIONS (url 'http://localhost:7474/db/data/cypher');

\dx
\dx+ neo4j_fdw
