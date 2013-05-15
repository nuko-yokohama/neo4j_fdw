CREATE FOREIGN TABLE bar2 (
    name text,
    follower_name text,
    follower_gender text)
    SERVER foo
    OPTIONS (query '{"query":"START n=node(*) MATCH p=n<-[]-fm RETURN n.name as name , fm.name  as follower_name, fm.gender? as follower_gender " }');

\pset null (null)
SELECT name, follower_name, follower_gender FROM bar2;

SELECT name, count(follower_name) as followes_count FROM bar2 GROUP BY name;


