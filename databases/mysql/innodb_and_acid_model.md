# InnoDB and the ACID Model

## Overview

Ref: https://dev.mysql.com/doc/refman/8.0/en/mysql-acid.html

对于ACID的具体的含义, 此处不再赘述, 只有我认为重要的点:

### Atomicity, A

主要与InnoDB的事务相关

- Autocommit setting
- BEGIN, COMMIT, ROLLBACK 事务相关statement

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
