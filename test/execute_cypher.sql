SELECT exec_cypher('http://localhost:7474/db/data/cypher', '{"query" : "START n=node(*) RETURN n.name as name, n.gender? as gender"}');
SELECT exec_cypher('http://localhost:7474/db/data/cypher', '{"query" : "START n=node(*) RETURN n.name as name, n.gender? as gender"}')->'columns' as columns;
SELECT exec_cypher('http://localhost:7474/db/data/cypher', '{"query" : "START n=node(*) RETURN n.name as name, n.gender? as gender"}')->'data' as data;
SELECT exec_cypher('http://localhost:7474/db/data/cypher', '{"query" : "START n=node(*) RETURN n.name as name, n.gender? as gender"}')->'data'->0 as data_row;

SELECT exec_cypher('http://localhost:7474/db/data/cypher', '{"query":"START n=node(*) MATCH p=fm<-[:follow]-n<-[:follow]-fm  RETURN n.name as my_name, n.gender as my_gender, fm.name  as follower_name, fm.gender as follower_gender" }');
SELECT exec_cypher('http://localhost:7474/db/data/cypher', '{"query":"START n=node(*) MATCH p=fm<-[:follow]-n<-[:follow]-fm  RETURN n.name as my_name, n.gender as my_gender, fm.name  as follower_name, fm.gender as follower_gender" }')->'columns' as columns;
SELECT exec_cypher('http://localhost:7474/db/data/cypher', '{"query":"START n=node(*) MATCH p=fm<-[:follow]-n<-[:follow]-fm  RETURN n.name as my_name, n.gender as my_gender, fm.name  as follower_name, fm.gender as follower_gender" }')->'data'->1 as data_row;

