https://dev.mysql.com/doc/refman/8.0/en/order-by-optimization.html

1. order by null
2. Use of Indexes to Satisfy ORDER BY
3. Use of filesort to Satisfy ORDER BY

example:

ALTER TABLE t_test add index key_1_2_idx (key_part1, key_part2);

如下order by 可能会全表扫"主键索引".

```sql
SELECT * FROM t1 ORDER BY key_part1, key_part2;
```

使用key_1_2_idx的场景:

```sql
SELECT pk, key_part1, key_part2 FROM t1 ORDER BY key_part1, key_part2;
```

```sql
SELECT * FROM t1 WHERE key_part1 = constant ORDER BY key_part2;
```

如下order by同样不一定使用索引 key_1_2_idx:

```sql
// 在索引上的range...不一定用索引树排序
SELECT * FROM t1 WHERE key_part1 > constant ORDER BY key_part1 ASC;
```

Use of filesort to Satisfy ORDER BY

https://dev.mysql.com/doc/refman/8.0/en/limit-optimization.html
