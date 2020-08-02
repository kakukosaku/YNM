# Locking Reads

如果你在一个事务内: query data -> insert or update data, 常规的`SELECT`语句并不能提供足够的保护. 其它事务仍然可以更新/删除, 你刚query的记录!

InnoDB支持2类型的locking reads以提供额外的保护:

- SELECT ... FOR SHARE

以共享锁的方式读(在事务持续期间, 其它session可读, 不可修改, 直到事务提交). 在获取该记录的读锁时, 如果有其余事务对记录的修改尚未提交, block直到其它事务提交.

- SELECT ... FOR UPDATE

与UPDATE语句有类似的功效, 以排它锁的方式锁住记录. 在事务持续期间, 其它事务是否可见, 取决于isolation level. 比如: consistent read ignore any locks set on the records.

所有 FOR SHARE or FOR UPDATE 查询所加的锁在事务提交或回滚后释放!

外层查询的Locking read不会锁子查询, 除非也在子查询上显示指定Locking read. e.g:

```sql
SELECT * FROM t1 WHERE c1 = (SELECT c1 FROM t2 FOR UPDATE) FOR UPDATE;
```

**Locking Read Example**

假设以下场景, 你有2张表, 一张叫child, 另一张叫parent. 你需要确保每个child中的row, 在parent表中都有一个parent row.

如果以事务A: 1. 先用consistent read(non-lock), 发现有 2. 再插入child row. 看似没有问题, Aha...

但考虑: consistent read仅生成read view, 以决定不同事务的操作, 在当前事务中的可见性. 并不会阻止其它事务对数据修改(删除). 这将导致在事务A进行同时, 另一个session中的事务删除了parent row, 而事务A毫无感知.

解决方式: 在事务A中, 以共享锁的方式读 parent row, 以确保在该事务中的第二步是正确的! (其它事务的删除parent row会被阻塞!) e.g:

```sql
SELECT * FROM parent WHERE NAME = 'Jones' FOR SHARE;
```

另外一个场景, reading and incrementing the counter, first perform a locking read of the counter using `FOR UPDATE`, and then increment the counter, For example:

```sql
SELECT counter_field FROM child_codes FOR UPDATE;
UPDATE child_codes SET counter_field = counter_field + 1;
```

A SELECT ... FOR UPDATE reads the latest available data, setting exclusive locks on each row it reads. 因此, **它会给读到的记录上与 UPDATE 一样的锁(X锁)** (关于不同语句会加什么样的锁, 本系列也有说明, 参见README)

这样的语句可以工作, 但...很少这样用, In MySQL, 这样的工作可以由一条语句完成: 

```sql
UPDATE child_codes SET counter_field = LAST_INSERT_ID(counter_field + 1);
-- SELECT LAST_INSERT_ID();
```

**Locking Read Concurrency with NOWAIT and SKIP LOCKED**

如果你想查询语句在记录有写锁时, 不等待锁(立即返回); 又或者返回**除了被锁写的记录**也是可接受的, 你可以这样做:

- `NOWAIT`: A locking read that uses NOWAIT never waits to acquire a row lock. The query executes immediately, failing with an error if a requested row is locked.

```
SELECT counter_field FROM child_codes FOR UPDATE NOWAIT;
```

- `SKIP LOCKED`: A locking read that uses SKIP LOCKED never waits to acquire a row lock. The query executes immediately, removing locked rows from the result set.

```
SELECT counter_field FROM child_codes FOR UPDATE SKIP LOCKED;
```

> Queries that skip locked rows return an inconsistent view of the data. 

NOWAIT, SKIP LOCKED都只能用于行锁. 此外, 这2者对`statement based replication` is unsafe!
