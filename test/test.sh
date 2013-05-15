#!/bin/sh
echo "neo4j_fdw demo start"
dropdb n4j
createdb n4j
echo "resister neo4j_fdw, and CREATE FOREIGN SERVER"
psql n4j -e -f test1.sql

echo "retrieve all nodes."
psql n4j -e -f test2.sql

echo "output a list of people and the number of people follow to in each user."
psql n4j -e -f test3.sql

echo "output the couple of the user of both follow."
psql n4j -e -f test4.sql

echo "execute_cypher function."
psql n4j -e -f execute_cypher.sql

echo "neo4j_fdw demo end"
