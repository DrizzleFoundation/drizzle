# include/index_merge2.inc
#
# Index merge tests
#
# The variable
#     $engine_type       -- storage engine to be tested
# has to be set before sourcing this script.
#
# Note: The comments/expectations refer to InnoDB.
#       They might be not valid for other storage engines.
#
# Last update:
# 2006-08-02 ML test refactored
#               old name was t/index_merge_innodb.test
#               main code went into include/index_merge2.inc
#

--echo #---------------- Index merge test 2 -------------------------------------------

eval SET SESSION STORAGE_ENGINE = $engine_type;

--disable_warnings
drop table if exists t1,t2;
--enable_warnings

create table t1
(
  key1 int not null,
  key2 int not null,

  INDEX i1(key1),
  INDEX i2(key2)
);

--disable_query_log
let $1=200;
begin;
while ($1)
{
  eval insert into t1 values (200-$1, $1);
  dec $1;
}
commit;
--enable_query_log

# No primary key
explain select * from t1 where key1 < 5 or key2 > 197;

select * from t1 where key1 < 5 or key2 > 197;

explain select * from t1 where key1 < 3 or key2 > 195;
select * from t1 where key1 < 3 or key2 > 195;

# Primary key as case-sensitive string with \0s.
# also make primary key be longer then max. index length of MyISAM.
alter table t1 add str1 char (100) not null,
                add zeroval int not null default 0,
                add str2 char (100) not null,
                add str3 char (100) not null;

update t1 set str1='aaa', str2='bbb', str3=concat(key2, '-', key1 div 2, '_' ,if(key1 mod 2 = 0, 'a', 'A'));

alter table t1 add primary key (str1, zeroval, str2, str3);

explain select * from t1 where key1 < 5 or key2 > 197;

select * from t1 where key1 < 5 or key2 > 197;

explain select * from t1 where key1 < 3 or key2 > 195;
select * from t1 where key1 < 3 or key2 > 195;

# Test for BUG#5401
drop table t1;
create table t1 (
  pk    integer not null auto_increment primary key,
  key1  integer,
  key2  integer not null,
  filler char  (200),
  index (key1),
  index (key2)
);
show warnings;
--disable_query_log
let $1=30;
begin;
while ($1)
{
  eval insert into t1 (key1, key2, filler) values ($1/4, $1/8, 'filler-data');
  dec $1;
}
commit;
--enable_query_log
explain select pk from t1 where key1 = 1 and key2 = 1;
select pk from t1 where key2 = 1 and key1 = 1;
select pk from t1 ignore index(key1,key2) where key2 = 1 and key1 = 1;

# More tests for BUG#5401.
drop table t1;
create table t1 (
  pk int primary key auto_increment,
  key1a  int,
  key2a  int,
  key1b  int,
  key2b  int,
  dummy1 int,
  dummy2 int,
  dummy3 int,
  dummy4 int,
  key3a  int,
  key3b  int,
  filler1 char (200),
  index i1(key1a, key1b),
  index i2(key2a, key2b),
  index i3(key3a, key3b)
) ENGINE=myisam;

create table t2 (a int);
insert into t2 values (0),(1),(2),(3),(4),(NULL);

insert into t1 (key1a, key1b, key2a, key2b, key3a, key3b)
  select A.a, B.a, C.a, D.a, C.a, D.a from t2 A,t2 B,t2 C, t2 D;
insert into t1 (key1a, key1b, key2a, key2b, key3a, key3b)
  select key1a, key1b, key2a, key2b, key3a, key3b from t1;
insert into t1 (key1a, key1b, key2a, key2b, key3a, key3b)
  select key1a, key1b, key2a, key2b, key3a, key3b from t1;
analyze table t1;
select count(*) from t1;

explain select count(*) from t1 where
  key1a = 2 and key1b is null and  key2a = 2 and key2b is null;

select count(*) from t1 where
  key1a = 2 and key1b is null and key2a = 2 and key2b is null;

explain select count(*) from t1 where
  key1a = 2 and key1b is null and key3a = 2 and key3b is null;

select count(*) from t1 where
  key1a = 2 and key1b is null and key3a = 2 and key3b is null;

drop table t1,t2;

# Test for BUG#8441
create table t1 (
  id1 int,
  id2 date ,
  index idx2 (id1,id2),
  index idx1 (id2)
);
insert into t1 values(1,'20040101'), (2,'20040102');
select * from t1  where id1 = 1  and id2= '20040101';
drop table t1;

# BUG#21277: Index Merge/sort_union: wrong query results
create table t1
(
  key1 int not null, 
  key2 int not null default 0,
  key3 int not null default 0
);

insert into t1(key1) values (1),(2),(3),(4),(5),(6),(7),(8);

let $1=7;
set @d=8;
begin;
while ($1)
{
  eval insert into t1 (key1) select key1+@d from t1;
  eval set @d=@d*2;
  dec $1;
}
commit;

alter table t1 add index i2(key2);
alter table t1 add index i3(key3);
update t1 set key2=key1,key3=key1;

# to test the bug, the following must use "sort_union":
explain select * from t1 where (key3 > 30 and key3<35) or (key2 >32 and key2 < 40);
select * from t1 where (key3 > 30 and key3<35) or (key2 >32 and key2 < 40);
drop table t1;