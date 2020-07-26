# Explain of MySQL's `EXPLAIN` statement

Ref: https://dev.mysql.com/doc/refman/5.5/en/explain-output.html

## Overview:

`EXPLAIN` statement provides information about the execution plan for a SELECT statement.

Usage:

EXPLAIN SELECT ... (SHOW WARNINGS if error occur)

Output Columns:

| Column        | Meaning                                        |
|---------------|------------------------------------------------|
| id            | The SELECT identifier                          |
| select_type   | The SELECT type                                |
| table         | The table for the output row                   |
| partitions    | The matching partitions                        |
| type          | The join type                                  |
| possible_type | The possible indexes to choose                 |
| key           | The index actually chosen                      |
| key_len       | The length of the chosen key                   |
| ref           | The columns compared to the index              |
| rows          | Estimate of rows to be examined                |
| filtered      | Percentage of rows filtered by table condition |
| extra         | Additional information                         |

## Detail

- id

SELECT 语句的序号, 可NULL(当所过滤的row为其它row的union结果时, 此时显示为`<union M,N>`)

- select_type

SELECT 语句的类型, 可以为以下:

| selecty_type Value | Meaning |
| ------------------ | ------- |
| SIMPLE             | 未使用 UNION 或 subqueries |
| PRIMARY            | 最外围的 SELECT |
| SUBQUERY           | First SELECT in subquery(第一个查询的搜索集为第二个查询结果) |
| DEPENDENT SUBQUERY | First SELECT in subquery, dependent on outer query (第一个查询的搜索集为第二个查询结果, 而第二个查询要使用第一个查询的条件) |
| DERIVED            | Derived table(子查询的结果集应用在 From 中, 作为前一个查询的驱动表) |
| UNION              | Second or later SELECT statement in a UNION(包含 UNION / UNION ALL的内层小查询) |

more type pass...

- table: table name

- partitions: match partitions

- type

| type Value      | Meaning |
| ----------------| ------- |
| system          | The table has only one row (= system table). This is a special case of the const join type. |
| const           | The table has at most one matching row, which is read at the start of the query. |
| eq_ref          | One row is read from this table for each combination of rows from the previous tables. |
| ref             | All rows with matching index values are read from this table for each combination of rows from the previous tables. |
| fulltext        | The join is performed using a fulltext index |
| ref_or_null     | 与ref类似, 但MySQL会额外搜索含NULL的值(对 key1 = const or key1 IS NULL)的优化 |
| index_merge     | 对 `SELECT * FROM tbl_name WHERE key1 = 10 OR key2 = 20` 的优化 |
| unique_subquery | value in `eq_ref` |
| index_subquery  | value in `ref` |
| range           | Only rows that are in a given range are retrieved, using an index to select the rows. |
| index           | 索引树全扫描 |
| ALL             | 全表扫描 |

summary:

1. system, const 都是只有一条结果返回 is used when you compare all part of a `PRIMARY KEY` or `UNIQUE` index to constant value. e.g. query:

```sql
SELECT * FROM tbl_name WHERE primary_key=1;

SELECT * FROM tbl_name
  WHERE primary_key_part1=1 AND primary_key_part2=2;
```

2. eq_ref 与 ref 的区别在于使用的索引"值"是否"唯一". 而 ref 与 ref_or_null 的区别在于`MySQL does an extra search for rows that contain NULL values.`

```sql
-- eq_ref
SELECT * FROM ref_table,other_table
  WHERE ref_table.key_column=other_table.column;

SELECT * FROM ref_table,other_table
  WHERE ref_table.key_column_part1=other_table.column
  AND ref_table.key_column_part2=1;
```

```sql
-- ref_or_null
SELECT * FROM ref_table
  WHERE key_column=expr OR key_column IS NULL;
```

3. index 与 ALL: index是指仅需要扫描全部的索引记录就好, 相比ALL全表扫描代价小.

```sql
value IN (SELECT primary_key FROM single_table WHERE some_expr)
```

- possible_keys, key, key_len, ref, rows, filtered show as table info...

- extra

一些常见的extra:

1. Using index: 扫描index tree时的提示
