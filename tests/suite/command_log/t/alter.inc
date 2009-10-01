# 
# Simple test of the serial event log for testing ALTER command 
# 
# We create a table then alter it by adding a column into it. 
# 
# We then use the transaction_reader in drizzled/message/ to read the events.
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

ALTER TABLE t1 ADD dummy INT;