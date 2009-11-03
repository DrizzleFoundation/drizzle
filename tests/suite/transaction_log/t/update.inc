# 
# Simple test of the serial event log for UPDATE statements
# 
# We create a table and insert some records
# into it.  We then update the table.
# 
#

--disable_warnings
DROP TABLE IF EXISTS t1;
--enable_warnings

CREATE TABLE t1 (
  id INT NOT NULL
, padding VARCHAR(200) NOT NULL
, PRIMARY KEY (id)
);

INSERT INTO t1 VALUES (1, "I love testing.");
INSERT INTO t1 VALUES (2, "I hate testing.");

# Simple PK update
UPDATE t1 SET padding= "XXX" WHERE id= 1;

# UPDATE all records in table
UPDATE t1 SET padding= "AAA";

DROP TABLE t1;

# Test for LP Bug#440141:
#
# Replication generates incorrect update commands when 
# where clause uses a field contained in set clause
#
CREATE TABLE t1 (
  id int AUTO_INCREMENT NOT NULL PRIMARY KEY
, name varchar(1024)
, alias varchar(1024)
);

INSERT INTO t1 (name,alias) VALUES ("jeff lebowski","dude");

UPDATE t1 SET alias = "the dude" WHERE alias = "dude";

DROP TABLE t1;