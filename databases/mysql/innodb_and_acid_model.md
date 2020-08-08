# InnoDB and the ACID Model

## Overview

Ref:

1. https://dev.mysql.com/doc/refman/8.0/en/mysql-acid.html
2. https://www.dummies.com/programming/php/making-sure-mysql-database-acid-compliant/

对于ACID的具体的含义, 此处不再赘述, 只写我认为重要的点:

### Atomicity, A

- 要么事务成功(所有操作均执行成功), 要么事务失败(所有操作均不会执行).
- Autocommit setting
- BEGIN, COMMIT, ROLLBACK 事务相关statement

MySQL 使用 two-phase commit approach to committing transactions:

1. Prepare phase: A transaction is analyzed to determine if the database is able to commit the entire transaction.
2. Commit phase: The transaction is physically committed to the database.

> The two-phase commit approach allows MySQL to test all transaction commands during the prepare phase without having to modify any data in the actual tables. 
> Table data is not changed until the commit phase is complete.

### Consistency, C

主要与InnoDB内部如何处理数据以保护数据, 免受各种crashes的影响, 保证数据始终保持一致!

- InnoDB doublewrite buffer.
- InnoDB crash recovery.

### Isolation, I

主要与InnoDB事务相关

- Autocommit setting.
- SET ISOLATION LEVEL statement.
- The low-level details of InnoDB locking.

额外的文章[I(isolation) of ACID](isolation_level.md)

### Durability, D

主要是MySQL与particular hardware交互相关. 基于CPU不同指令集, 网络和存储设备, 这是本guideline最复杂的部分, Related MySQL features include:

- InnoDB doublewrite buffer 及相关配置
- sync_binlog 的相关配置
- innodb_file_per_table 相关配置
- Write buffer in a storage device, such as a disk drive, SSD, or RAID array.
- Battery-backed cache in a storage device.
- 操作系统的 `fsync()` 调用的支持
- 无间断的电源供给
- 备份策略, 如备份频率, 备份类型, 及保留时间
- 对分布式应用, 数据中心硬件的位置, 数据中心间网络的情况等

可以看到 D 与众多因素相关...
