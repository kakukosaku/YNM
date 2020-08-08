# Distributed Transaction Protocol/Algorithm

## Overview

- [Two Phase Commit, 2PC](#2pc)
- [Three Phase Commit, 3PC](#3pc)

### 2PC

二阶段提交协议是将事务提交过程分成了两个阶段来进行处理的, 其执行流程如下:

- 提交事务请求, 也称为投票阶段

1. 事务询问, coordinator向事务participator询问是否可以执行事务提交操作,, 并开始等待各参与响应
2. 执行事务, 各participator执行事务操作, 写redo, undo. (实际仍未提交)
3. 反馈结果, 各participator向coordinator反馈执行事务结果, 成功Yes, 否则No

- 执行事务提交, 真正commit or rollback

如果事务参与者都给出yes:

1. 协调者通知各参与者commit事务
2. 各参与者在commit后, 释放事务占据资源
3. 同时ACK协调者
4. 协调者收到所有参考者ACK, 分布式事务完成

任何一参与者给出了No, 或者在等待超时后, 协议调仍未能收到所与参与者的"反馈结果"中断事务:

1. 协议者通知各参与者事务回滚
2. 各参与者回滚事务
3. 同时ACK协调者
4. 协调者收到所有参考者ACK, 分布式事务中断

**优缺点**

优:

原理简单, 实现方便

缺:

- 同步阻塞: 所有参与者在都在等待各参与者的"反馈结果", 由协调者收集完/超时, 再通知参与者下步行动
- 单点问题: 协调者作用巨大, 一旦GG 整个2PC失效, 如果发生在第2步, 还会导致参与者陷入事务锁定状态, 无法提交
- 数据不一致: 在第2步时, 如果网络分区发生, 部分参与者收到commit请求, 会有部分参与者提交...部分无法提交. 系统状态发生不一致
- 失败策略太过保守: 在第1步时, 任意参与者的failure/timeout, 都会导致2PC 事务中断.

**ZooKeeper Implementation**

Ref: https://zookeeper.apache.org/doc/current/recipes.html#sc_recipes_twoPhasedCommit

注意: 这个实现...比较粗陋, 但是文档上单独描述了, 还是有了解的必要的...

2阶段提交, 允许分布式系统中, 各client对于事务的提交/回滚达成一致(agree either to commit or abort).

利用 zk, 你可实现2阶段提交: 由协调者, coordinator create a transaction node, 如"/app/Tx", 每个参与者, participator site创建"/app/Tx/s_i". 当coordinator创建child node时, it leave the content undefined.

1. 当participator收到coordinator的事务请求时, the site reads each child node and sets a watch(on "/app/Tx").
2. 然后每个site处理自己的"query"并且 votes "commit" or "abort" by writing to its respective node. Once the write completes, the other sites are notified.
3. 所有site都vote完毕, they can decide either "abort" or "commit".

需要注意的是:

1. a node can decide "abort" earlier if some site votes for "abort".
2. 另外一个需要注意的是: 这个实现中zk仅创建了group site: "/app/Tx", 然后将事务propagate to the corresponding sites. 这是为了使用临时节点, ephemeral node来检测参与者failure.
3. 此外, 上面的描述还有个缺点, 每个site都关注子节点, 导致实际上是O(n**2)的操作. 可以改为由coordinator监听子节点的变化, 在达到commit/abort结论时再通知site

### 3PC

三阶段提交, 二阶段提交的改进版...它将2PC的第2步, 提交事务请求一分为2, 形成了3PC:

- Can Commit

1. 事务询问, coordinator向事务participator询问是否可以执行事务提交操作,, 并开始等待各参与响应.
2. 收集响应, 参与者向协调者回复, 是否可以顺便执行事务, 如果是Yes, 参与者进行预备状态, 否则反馈No.(注意, 此时参与者并不直接开启事务)

- PreCommit

如果协调者"收集响应"都为Yes, 则进入prepare阶段

1. 发送预提交请求, 协调者再向事务各参与者发出precommit
2. 事务预提交, 收到precommit的事务参与者, 开启各自事务, 锁定资源, 写redo, undo信息...
3. 反馈预提交结果, 事务各参与者, 回复协调者"事务预提交"结果, 同时等待最终指令: commit or abort.

如果任意一个参与者向协调者给出了No, 或者等待超时后, 协调者仍无法收集到全部反馈(`收集响应`或`反馈预提交结果`过程中), 那么中断事务

1. 发送中断请求, 协调者向所有参与者发送abort.
2. 中断事务, 无论是收到协调者的abort, 等待协调者过程中出现超时, 参与者都会中断事务.

- Do commit

该阶段真正执行事务提交

1. 发送提交请求, 协调者向所有参考者发送doCommit请求.
2. 事务提交, 参与者在收到doCommit后, 直接提交各自事务, 释放事务占据的资源.
3. 反馈事务提交结果, 参与者ACK协调者, 事务提交结果
4. 协调者收到所有事务参与者的ACK, 事务完成

若协调者收到NACK或者超时, 无法收到事务参与者的"事务提交结果", 开始中断事务

1. 发送中断请求, 协调者给所有参与者发送abort请求
2. 事务回滚, 收到abort的参与者利用undo日志进行事务回滚
3. 反馈事务回滚结果, 参与者ACK协调者, 事务回滚结果
4. 中断事务, 协调者收到所有参与者ACK, 事务中断

同时需要强调的是, 进行第三阶段, doCommit时: 1. 协调者出现问题 2. 协调者与参与者出现网络分区 都会导致参与者无法doCommit or abort. 参与者通常会在等待超时后继续事务提交...

**优缺点**

优:

相较2PC, 降低了阻塞范围, 细分为 canCommit, PreCommit.

缺:

相较2PC, doCommit后, 出现网络分区 "参与者通常会在等待超时后继续事务提交", 而协调者做出abort决定后, 其它参与者回滚, 事实上仍会造成不一致的数据/状态.
