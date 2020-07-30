# Mulit-Version Concurrency Control, MVCC

Ref: https://www.valinv.com/dev/mysql-what-is-mysql-mvcc

## Overview

InnoDB 是一个 Multi-versioned storage engine, 可以提供诸如: 1. rollback 2. consistent read 等功能.

InnoDB add three fields to each row stored in the database.

1. DB_TRX_ID, 事务ID for last transaction that inserted or updated the row.
2. DB_ROW_ID, a row ID that increases monotonically as new rows are inserted.
3. DB_ROLL_PTR, 回滚指针, The roll pointer points to an undo log record written to the rollback segment.

Undo logs 每很多种(通过头部标识字节), 这里说2种 insert or update undo logs.

- insert undo log: 每在该插入事务回滚时需要, 所以可以在回滚时直接丢弃(discard).
- upload undo log: 除了事务本身回滚, 在consistent read中也需要(在其它事务中的consistent read生成read view中使用). 

InnoDB 会在当已分配的`snapshot`不再需要该update undo log时再丢弃.

所以, commit you transaction regularly, 否则InnoDB无法丢弃这些undo logs, it may grow too big, filling up yous tablespace.

在MVCC下, InnoDB不再直接物理删除你的`DELETE`的记录, 而是在该记录的`UPDATE undo log`(InnoDB将DELETE也视为UPDATE, 作标记性删除)被判断为无用, discard时, 再删除该记录与记录的index.

这就带来一个问题, 当你以相同的速率在表中插入删除少量的行时, 后台线程(purge thread)可能会工作滞后, 使得表逐渐变大...此时需要增加purge thread资源, 使用"回收"工作足够迅速.

以外, 针对 cluster index(也称primary index, 聚簇索引, 主键索引等)的更新与secondary index(也称辅助索引)的更新处理方式也不同:

- cluster index: 由于有前面说的三个hidden system columns point undo log可以用于重建更新前的数据, 所以它们的修改是原地修改, update in-place.
- secondary index: 并没有那三个隐藏列, 所以不能原地更新. 怎么做呢?

old secondary index 先被标记为删除(delete-mark), 新的记录被插入, 并且delete-marked rows are eventually purged.
这样consistent read时, 如果发现了delete-marked 记录, 回表拿`DB_TRX_ID`与当前事务比对, 决定可见性. (此时索引覆盖技术失灵...仍然会回表!)

但上述现象并不影响 [index condition pushdown(ICP)](https://dev.mysql.com/doc/refman/8.0/en/index-condition-pushdown-optimization.html), 依然可以在回表前比对其它secondary index compare.

这篇 MySQL 文档上对于 MVCC 机制如何工作并未做详细说明, 实际上MVCC机制不难理解:

1. 通过每个事务开始前申请递增的事务ID `DB_TRX_ID`
2. 在consistent read前生成 readView, 其中包括 1. 当前活跃事务id 2. 当前活跃最小ID 3. 当前事务启动时MySQL尚未分配的最大事务ID(其实就是当前事务之后的事务ID)
3. 根据隔离级别, 在 rollback pointer 所指向的 undo log 中事务ID的先后关系/提交与否, 决定其它事务对数据修改的可见性.
