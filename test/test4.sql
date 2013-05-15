DROP FOREIGN TABLE IF EXISTS bar3;
CREATE FOREIGN TABLE bar3 (
    my_name text,
    my_gender text,
    follower_name text,
    follower_gender text)
    SERVER foo
    OPTIONS (query '{"query":"START n=node(*) MATCH p=fm<-[]-n<-[]-fm  RETURN n.name as my_name, n.gender as my_gender, fm.name  as follower_name, fm.gender as follower_gender" }');

\pset null (null)
SELECT my_name, my_gender, follower_name, follower_gender FROM bar3;

