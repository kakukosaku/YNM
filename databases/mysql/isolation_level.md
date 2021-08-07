# I(isolation) of ACID

## Overview

Ref:

1. https://dev.mysql.com/doc/refman/8.0/en/innodb-transaction-isolation-levels.html
2. https://dev.mysql.com/doc/refman/8.0/en/set-transaction.html
3. https://dev.mysql.com/doc/refman/8.0/en/innodb-consistent-read.html
4. https://dev.mysql.com/doc/refman/8.0/en/innodb-locking.html
5. https://sqlperformance.com/2014/04/t-sql-queries/the-serializable-isolation-level
6. https://dev.mysql.com/doc/refman/8.0/en/innodb-next-key-locking.html

High level 的解释: 事务隔离级别讲的是"并发"事务之间的"可见性". 调整事务隔离级别, 可"精调"**数据可靠性, 一致性, 当多事务并发读/写时的结果
可再现性**与**性能**之间的平衡.

更进一步的解释 "事务隔离级别" 的重要意义:

1. 设想这样一个场景: 所有并发的事务全部以串行执行(即事务之间完全隔离, 互不影响), 但这样会导致本就互不影响的事务(如, 读不同记录的事务)也串行
执行, 性能大幅下降;
2. 现在允许事务"并发" **读**, 如果事务全部是只读, 不修改数据, 我们也能保证事务之间是"隔离影响"的, 即保证在事务中, 读到的数据不会发生变化
(因其它事务发生).
3. 现在再允许事务"并发"**写**, 如果事务全部是只写, 不读^.^, 我们通过加**排他锁**也算是能保证事务之间是"隔离影响"的, 即保证事务的执行与其它
事务无关.(修改相同数据的事务等待获取**排他锁**)
4. 事情再复杂些允许事务"并发"[ 读, 写 ], 问题的复杂度陡然上升, 如果我们对全部操作都加**排他锁**, 使得读多操作很吃亏...读的操作加**共享锁**
(也称分别为写锁/读锁), 又使得事务的执行效率太低, 并非所有场景都需要如此严格的"事务隔离级别".

问题: 频繁加/解锁会影响性能, 显著降低吞吐. 如何在性能与事务的(其它)特性之间取得平衡, 事务的隔离级别闪亮登场😃.

先解释4种事务隔离级, 及其适用场景(SQL标准中定义的), 再说MySQL(InnoDB 引擎)默认是什么样的

开始之前, 有些背景知识(一致性读, 有锁读, 锁)需要了解:

- MySQL 读: [consistent non-locking read](innodb_locking_and_transaction_model.md#consistent-nonlocking-reads) & 
[locking read](innodb_locking_and_transaction_model.md#locking-reads)(for update / for share)
- MySQL 锁: [InnoDB Locking](innodb_locking_and_transaction_model.md#innodb-locking)

### READ UNCOMMITTED

- 如果该事务的隔离级别为 `READ UNCOMMITED`, 读以无锁方式进行, 意味着, 这个读事务可能读到其它事务(注意, I是并发事务的隔离级别)未提交的修改,
若其它事务回滚, 数据复原(或数据再次被提交的事务修改), 这个事务的读, 实际读到了脏数据!
 
这样的现象称之为dirty read, 脏读. 背离了ACID的事务设计原则, 很少使用; 在非一致性数据可接受的场景下可以使用.

### READ COMMITTED

- 在事务READ COMMITED中, 只能读到已提交的修改(包含事务进行中被提交的).
- 在每个consistent read(一致读, 下不再翻译)中, 甚至于在事务中的consistent read, 它读自己快照(基于MVCC实现).
- 而对于locking reads(有锁读, 下不再翻译)(SELECT WITH FOR UPDATE or FOR UPDATE 语句), UPDATE语句, DELETE语句, InnoDB仅锁定被索引
的记录, 记录之间的"间隙"并不锁定.

因此向在该隔离级别下, 向被锁定的记录之间插入新的records是被允许的! Gap locking 仅在1. 外键约束检查 2. duplicate-key checking 时施加.

也正因此, 会产生[Phantom Rows](https://dev.mysql.com/doc/refman/8.0/en/innodb-next-key-locking.html), 幻影行: 前后2次同样的
Query, 产生的结果集不同!(多了幻影行...)

- 解决了dirty read, 但对于 phantom rows无能为力.

此外还有些更详细的描述:

- Only row-based binary logging is supported with the READ COMMITTED isolation level. If you use READ COMMITTED with
binlog_format=MIXED, the server automatically uses row-based logging.
- 对于UPDATE or DELETE statements, InnoDB仅锁要更新/删除的记录. Records locks for nonmatching rows are released after MySQL
has evaluated WHERE condition. 这极大的减少了死锁发生的可能性, 但仍有.
- 对于UPDATE statements, 如果记录已经被锁, InnoDB执行 semi-consistent read, 返回最新已提交的记录给MySQL用以判断是否满足WHERE, 如果
满足, 则尝试加锁或等待锁的释放.

对此有以下官方示例:

Suppose that one session performs an UPDATE using these statements:

```mysql
# Session A
START TRANSACTION;
UPDATE t SET b = 5 WHERE b = 3;
```

Suppose also that a second session performs an UPDATE by executing these statements following those of the first session:

```mysql
# Session B
UPDATE t SET b = 4 WHERE b = 2;
```

在`READ COMMITTED`隔离级别下, 当InnoDB执行每个UPDATE语句时, 它首先对每个记录获取"排它锁", 然后看看是否需要修改之. 如果不修改该记录释放之,
否则lock until the end of the transaction. 

在执行第一个UPDATE时, 有如下示意:

```
x-lock(1,2); unlock(1,2)
x-lock(2,3); update(2,3) to (2,5); retain x-lock
x-lock(3,2); unlock(3,2)
x-lock(4,3); update(4,3) to (4,5); retain x-lock
x-lock(5,2); unlock(5,2)
```

此时第二个UPDATE, InnoDB does a "semi-constent" read. 如下所述: 如下示意

```
x-lock(1,2); update(1,2) to (1,4); retain x-lock
x-lock(2,3); unlock(2,3)
x-lock(3,2); update(3,2) to (3,4); retain x-lock
x-lock(4,3); unlock(4,3)
x-lock(5,2); update(5,2) to (5,4); retain x-lock
```

而在`REPEATABLE READ`隔离级别下, the first UPDATE acquires an x-lock on the row that it reads and does not release any of them.

如下示意:

```
x-lock(1,2); retain x-lock
x-lock(2,3); update(2,3) to (2,5); retain x-lock
x-lock(3,2); retain x-lock
x-lock(4,3); update(4,3) to (4,5); retain x-lock
x-lock(5,2); retain x-lock
```

此时第二个UPDATE试图获取时的阻塞(由于第一个事务并未释放锁on all rows) and does not proceed until the first UPDATE commits or roll back.

如下示意:

```
x-lock(1,2); block and wait for first UPDATE to commit or roll back
```

额外, 正确理解 consistent nonlocking reads & locking reads(for update or for share) !

### REPEATABLE READ

- MySQL默认隔离级别.
- 对于consistent read, 在同一事务中的多次consistent read都基于第一次的读时建立的 snapshot.
- 对于locking reads(SELECT with FOR UPDATE or FOR SHARE), UPDATE, and DELETE statement, 加锁策略取决于statement是否使用
unique index 以及 WHERE 的query condition(是unique search or range-type search).

对于unique index with a unique search condition, InnoDB仅锁找到的记录, not the gap before it.

对于other condition, InnoDB锁range范围内的记录以及使用 gap locks or next-key locks 以阻止幻影行的插入. 参见[InnoDB Locking](innodb_locking_and_transaction_model.md#innodb-locking)

额外的, 对于该隔离级别下, 对Phantom row的"解决"仅限于consistency read(MVCC机制实现的无锁读), 关于此, 更详情的请看MVCC文档中, 关于例外的介绍.

### SERIALIZABLE

- `serializable`序列化, 或叫串行化是个容易误解的词. 对于"并发"与"并行"的区别我再简单强调下: 并发强调的了**时间段**内的"同时发生", 而并行
指的**某一时刻**上的"真正同时发生".

所以当并发事务, 以 `SERIALIZABLE` 的隔离级别运行时, 并非指真正意义上的串行化执行. 而是指事务(们)的执行结果不发生变化, 即使有部分overlap(重叠),
切换. 参考Ref(5)中更详细的讨论

- 当然, 加锁也仅仅是一种实现方式, Ref(5) 中也给出了PostgreSQL的参考实现: http://wiki.postgresql.org/wiki/SSI

### Last but not least

- MySQL 8.0: The default isolation level is REPEATABLE READ
- 事务隔离级别, 可以是全局的scope, 也可以是session scope, 以及下一个事务scope

如何修改事务隔离级别? global / session / Next Transaction, 见Ref(2)
