# Explain of MySQL's `EXPLAIN` statement

Ref:

1. https://dev.mysql.com/doc/refman/5.5/en/explain-output.html

---

`EXPLAIN` statement provides information about the execution plan for a SELECT statement.

Usage:

EXPLAIN SELECT ... (SHOW WARNINGS if error occur)

Output Columns:

1. id

SELECT 语句的序号, 可NULL(当所过滤的row为其它row的union结果时, 此时显示为`<union M,N>`)

2. select_type

SELECT 语句的类型, 可以为以下:

| selecty_type Value | Meaning |
| ------------------ | ------- |
| SIMPLE | 未使用 UNION 或 subqueries |
| PRIMARY | 最外围的 SELECT |
| SUBQUERY | First SELECT in subquery(第一个查询的搜索集为第二个查询结果) |
| DEPENDENT SUBQUERY | First SELECT in subquery, dependent on outer query (第一个查询的搜索集为第二个查询结果, 而第二个查询要使用第一个查询的条件) |
| DERIVED | Derived table(子查询的结果集应用在 From 中, 作为前一个查询的驱动表) |
| UNION | Second or later SELECT statement in a UNION(包含 UNION / UNION ALL的内层小查询) |

更多type 参照Ref(1), ps: 翻译好难..

3. table

表名

4. partitions

匹配的分区

5. type

| type Value | Meaning |
| ------------------ | ------- |
| system | The table has only one row (= system table). This is a special case of the const join type. |
| const | 表中最多只一条匹配数据, 用在对(all parts of)PRIMARY KEY 或 UNIQUE INDEX 查询中 |
| eq_ref | 与const/system类似(区别为: one row is read from this table for each combination of rows from the previous tables), all parts of an index are used by the join and the index is a `PRIMARY KEY` or `UNIQUE NOT NULL` index. |
| ref | 与eq_ref类似, 区别在于: the `ref` is used if the join uses only a leftmost prefix of the key or if the key is not a `PRIMARY KEY` or `UNIQUE` index, in other words, if the join cannot select a single row based on the key value. |
| fulltext | The join is performed using a fulltext index |
| ref_or_null | 与ref类似, 但MySQL会额外搜索含NULL的值(对 key1 = const or key1 IS NULL)的优化 |
| index_merge | 对 `SELECT * FROM tbl_name WHERE key1 = 10 OR key2 = 20` 的优化 |
| unique_subquery | This type replaces eq_ref for some IN subqueries of the following form: `value IN (SELECT primary_key FROM single_table WHERE some_expr)` |
| index_subquery | This join type is similar to unique_subquery. It replaces IN subqueries, but it works for nonunique indexes in subqueries of the following form: `value IN (SELECT key_column FROM single_table WHERE some_expr)` |
| range | Only rows that are in a given range are retrieved, using an index to select the rows. |
| index | The index join type is the same as ALL, except that the index tree is scanned. 出现场景: 1. 所需数据在辅助索引中可满足. 2. 通过扫描辅助索引树, 找主键索引(这种在extra中不会出现Uses index). 不要看type是index就觉得用索引了会很快, 实现它只比ALL全表扫描好一点点, 它只扫描了辅助索引树! |
| ALL | 全表扫描 |

6. possible_keys

The possible indexes to choose

7. key

The index actually chosen

8. key_len

The length of the chosen key

9. ref

The columns compared to the index

10. rows

Estimate of rows to be examined

11. filtered

Percentage of rows filtered by table condition

12. Extra

This column contains additional information about how MySQL resolves the query.

