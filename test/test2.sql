CREATE FOREIGN TABLE bar1 (
    name text,
    gender text,
    location text)
    SERVER foo
    OPTIONS (query '{"query":"START n=node(*) RETURN n.name as name, n.gender? as gender, n.location? as locatoin " }');

\pset null (null)
SELECT name, gender, location FROM bar1;
SELECT name, gender FROM bar1 WHERE location = 'Yokohama';
