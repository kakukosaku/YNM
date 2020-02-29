# Isolation of ACID (Atomicity, Consistency, Isolation, Durability)

Ref:

1. https://dev.mysql.com/doc/refman/8.0/en/innodb-transaction-isolation-levels.html
2. https://dev.mysql.com/doc/refman/8.0/en/set-transaction.html
3. https://dev.mysql.com/doc/refman/8.0/en/innodb-consistent-read.html
4. https://dev.mysql.com/doc/refman/8.0/en/innodb-locking.html
5. https://sqlperformance.com/2014/04/t-sql-queries/the-serializable-isolation-level
6. https://dev.mysql.com/doc/refman/8.0/en/innodb-next-key-locking.html

首先原子性(A), 一致性(C), 持久性(D)都是比较直接理解的; 事务隔离级别(I)的意义是什么呢? MySQL官方文档对于I的解释是:

> The isolation level is the setting that fine-tunes the balance between performance and reliability, consistency, and reproducibility of results when multiple transactions are making changes and performing queries at the same time.

事务隔离级别是一个fine-tune(精细调优), 对性能与"可靠的,一致的可再现的结果"当多个事务(修改或读)并发时的调优, 明白的一看就懂, 不明白的就感觉不太像在说人话(字懂意不解...)

>  ... If we regard a set of rows as a data item, the new phantom child would violate(违背) the isolation principle of transactions that a transaction should be able to run so that **the data it has read does not change during the transaction**.

幻行(Phantom row)违背了一个事务得以执行的隔离原则(什么原则->它所读到的数据not change在事务中)!

好, 现在设想这样一个场景, 如果所有事务全部以串行执行(即事务之间完全隔离, 互不影响), 但这样会导致本就不相干的事务(如, 读不同记录的事务)也串行执行, 性能大幅下降;

好, 现在允许事务"并发"[ 读 ], 如果事务全部是只读, 不修改数据, 我们也能保证事务之间是"隔离影响"的, 即保证在事务中, 读到的数据不会发生变化(因其它事务发生).

好, 现在再允许事务"并发"[ 写 ], 如果事务全部是只写, 不读^.^, 我们通过加**排他锁**也算是能保证事务之间是"隔离影响"的, 即保证事务的执行与其它事务无关.(修改相同数据的事务等待获取**排除锁**)

好, 事情再复杂些允许事务"并发"[ 读, 写 ], 问题的复杂度陡然上升, 如果我们对全部操作都加**排他锁**, 使得读多操作很吃亏...读的操作加**共享锁**(也称分别为写锁/读锁), 又使得事务的执行效率太低, 并非所有场景都需要如此严格的"事务隔离级别".

好, 众所周知, 加锁会影响性能, 如何在性能与事务的(其它)特性之间取得平衡, 事务的隔离级别闪亮登场😃. (感觉事务的内容环环相扣, 其它知识本文暂不介绍)

先解释4种事务隔离级, 及其适用场景(SQL标准中定义的), 再说MySQL(InnoDB 引擎)默认是什么样的

开始之前, 需要知道 MySQL 采用MVCC做并发控制. 读分为 consitent nonlocking read & locking read(for update for share), 关于锁的详细介绍会再开一篇!

1. READ UNCOMMITTED

> SELECT statements are performed in a nonlocking fashion, but a possible earlier version of a row might be used. Thus, using this isolation level, such reads are not consistent. This also called a dirty read.

如果该事务的隔离级别为 `READ UNCOMMITED`, 读以无锁方式进行, 意味着, 这个读事务可能读到其它事务(注意, I是并发事务的隔离级别)未提交的修改, 若其它事务回滚, 数据复原, 这个读事务, 实际读到了脏数据!  解决 dirty read -> READ COMMITED.

适用: 一致性数据&repeatable result 没有比**降低**加锁引起负载重要的场景. 

2. READ COMMITTED

> Each consistent read, even within the same transaction, sets and reads its own fresh snapshot. For information about consistent reads, see Ref(3)

> For locking reads (SELECT with FOR UPDATE or FOR SHARE), UPDATE statements, and DELETE statements, InnoDB locks only index records, not the gaps before them, and thus permits the free insertion of new records next to locked records.

> Because gap locking is disabled, phantom problems may occur, as other sessions can insert new rows into the gaps. For information about phantoms, see Section 15.7.4, “Phantom Rows”.

通过 consistent read, 保证READ COMMITED, 在事务中, 只能读到事务开始时, 已提交的修改 & 事务中做出的修改, 强烈建议阅读Ref(3)

对于locking reads, UPDATE, DELETE, InnoDB, 仅锁定(要)索引的记录, 之间的"间隙"并不锁定. 由于 gaps lock is disable, 幻影行(phantom rows)可能被插入! 导致同一事务2次读, 返回数据行数不一致, 不可重复读! 解决 -> REPEATABLE READ

更多, 强烈建议阅读Ref(1): 

对UPDATE OR DELETE InnoDB 仅锁相关行记录, 如果行记录的不符合MySQL 对WHERE conditon的计算(注意,并非SQL语句的单纯的WHERE), 锁被释放.

对UPDATE, 如果行本身已被锁定, InnoDB执行半一致性读(semi-consistent read), 返回最新的committed version, 看是否与当前事务的WHERE conditon(版本号等)相符; 相符MySQL继续获得锁/等待锁

额外, 正确理解 consistent nonlocking reads & locking reads(for update or for share) !

3. REPEATABLE READ

>  Consistent reads within the same transaction read the snapshot established by the first read. This means that if you issue several plain (nonlocking) SELECT statements within the same transaction, these SELECT statements are consistent also with respect to each other. See Ref(3).

> For locking reads (SELECT with FOR UPDATE or FOR SHARE), UPDATE, and DELETE statements, locking depends on whether the statement uses a unique index with a unique search condition, or a range-type search condition.

> For a unque index with a unique search condition, InnoDB locks only the index record found, not the gap before it.

> For other search conditions, InnoDB locks the index range scanned, using gap locks or next-key locks to block insertions by other sessions into the gaps covered by the range. For information about gap locks and next-key locks, see Ref(4).

懒的翻译了, 总结一下: 在同一事务, 非加锁的读(多次), 是建立在第一次读时的快照基础上, 保证读的一致性(详细参见Ref(3)); 对于有锁读, 根据是否使用索引及索引类型加锁(是否使用gap locks or next-key lock等)用以block其它session的插入 -> 解决幻读问题, 但可能读不到最新数据.

强烈建议阅读 consistent read Ref(4), 本系列也会增加对4的总结升华:)

另外, 很多文章对于幻行产生的描述略有歧义, 幻行产生实质是在 READ COMMITED 的隔离级别下, 事务加锁的方式(仅锁用到的行), 对于行"间隙"未加锁, 导致事物执行期间插入了新的数据, 而非事务执行期间, 其它事务的修改可见! (有些地方错误的以图示A, B事务, B先完成的修改, 导致A可见. 实际仅在限定情况下如此, 前述!)

4. SERIALIZABLE

正确理解 `serializable`, 是个问题. 经常看文档的可能也知道另一个词 serialize, 经常翻译为序列化, serializable, 翻译为可序列化的似乎没什么毛病, 但总感觉不像在说人话...我暂且叫它"串行化", 请接着看:)

事实上, SQL-92(SQL标准中对其有以下描述), 强烈建议阅读上 Ref(5):

> A serializable execution is defined to be an execution of the operations of concurrently executing SQL-transactions that produces the **same effect as some serial execution** of those same SQL-transactions. A serial execution is one in which each SQL-transaction executes to completion before the next SQL-transaction begins.

Ref(5)中也解释到:

> There is an important distinction to be made here between truly serialized execution (where each transaction actually runs exclusively to completion before the next one starts) and serializable isolation, where transactions are only required to have the same effects as if they were executed serially (in some unspecified order).

> To put it another way, a real database system is allowed to physically overlap the execution of serializable transactions in time (thereby increasing concurrency) so long as the effects of those transactions still correspond to some possible order of serial execution. In other words, serializable transactions are potentially serializable rather than being actually serialized.

> Locking is just one of the possible physical implementations of the serializable isolation level.

事务的执行, **好似**在串行执行, 关键在于对资源"加锁"的方式, 像READ COMMITED, 用基于snapshot的consistent read & locking read -> 解决脏读; 又或是像REPEATABLE READ next-key lock & 更新snapshot -> 解决幻行; 但又引入了读不到最新数据的问题 ... 而SERIALIABLE解决了事务中, 读的是最新数据! 

其实都是一步步的"升级锁", 至于MySQL是以何种实现SERIALIZABLE隔离级别, 待续...但你需要知道, 该隔离级别一定是满足了如文一开始对事务隔离的描述, 且给与一定灵活性, **并非一般意义的事务串行**! Ref(5) 中也给出了postgresql的参考实现 http://wiki.postgresql.org/wiki/SSI

5. MySQL 8.0: The default isolation level is REPEATABLE READ

如何修改事务隔离级别? global / session / Next Transaction, 见Ref(2)

