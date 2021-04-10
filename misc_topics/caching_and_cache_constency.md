# Different ways of caching and maintaining cache consistency

Ref:

1. https://blog.the-pans.com/different-ways-of-caching-in-distributed-system/
2. https://danielw.cn/cache-consistency-with-database

> There are two hard things in computer science: cache invalidation, naming things.

在分布式系统CAP理论中, 由于网络的天然不可信, 网络分区一定会形成(P), 为提高系统可用性(A), 必然有数据的冗余备份需求, 多备份的数据如何保证一致(C), 成为各系统设计需要平衡的(trade off).

在简单的缓存使用场景下, 如何保障缓存(cache)与数据源(db)数据一致, 实事上也是一个重要命题. 但大多数场景下, 自动过期的缓存, 加上不严格要求一致的场景, 使得这个问题很容易不被重视. 本文即细致的讨论: 如何维护cache与db的一致性.

### 定义问题

在计算机存储层次结构理论中, 我们知道, 多层存储层次(上层可以理解为下层的"高速缓存")是为解决不同类型存储设备在容量, 访问速度, 成本的上差别. 

同样, cache与db间也是这样的关系. 一般而言, cache访问更快, 数据量是db的子集.

在访问(read)时, 一般先访问cache, 未命中再访问db. 当然, 也有cache是db的全集, 甚至超集(数据写cache, "异步"落db), 这种情况, 只访问cache就足够了.

在有数据更新(write)时, 如何**维护好2份数据的一致性(C)**, 是个重要的问题.

## 解决方案

概括的讲有2类cache方案: 

1. Look-aside/demand-fill cache

```text
        |-----------|
        |   Client  |
        |-----------|  
 (2) /  / (3)      \ (1)
    /  /            \
|--------|          |--------|
|   DB   |          |  Cache |
|--------|          |--------|
```

在该种缓存方案中:

(1). 先访问cache, 命中直接返回, 未命中继续(2)

(2). 访问db, 根据是否要再写回cache决定是否要(3)

(3). 该未命中的数据写回cache

2. write-through/read-through cache

```text
        |-----------|
        |   Client  |
        |-----------|  
            |  |
        |-----------|
        |   Cache   |
        |-----------|  
            |  |
        |-----------|
        |   DB      |
        |-----------|  
```

与1方案, 区别在于cache和db的大小关系; 以及cache对数据更新的职责大小. 不细述.

以简单look-aside & demand-fill为例, 有可能使得cache与db出现数据不一致.

根据具体使用场景, (3)的具体形式会有不同方案实现:

| Impl | Question |
|------|----------|
| write-through, 同时更新cache && db: db->cache; cache->db | 提供更好的一致性, 但增加了并发更新时问题的复杂度: (将cache数据的更新事实上带入了其它问题中~) |
| write-back, 先更新cache, 再异步更新db; 或反过来 | 在异步更新的时延内, 事实上存在2份不一致的数据! |
| write-invalidate, 先更新db, 再失效缓存 | 依赖读时穿透更新缓存, 或用异步更新解决读写并发时的不一致问题. |

实际上, 根据业务场景的不同, 我们可以选择完全不同的方案, 例如: 后台业务上仅需要cache 7天内的数据, 且在数据刚更新后的时段内, 业务上能接受较短的查询时延. 我们完全可以使用MQ异步同步近期的db数据更新(bin log)至cache(甚至离线表中).
