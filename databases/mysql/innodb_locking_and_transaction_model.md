# InnoDB Locking and Transaction Model

## Overview

Ref: https://dev.mysql.com/doc/refman/8.0/en/innodb-locking-transaction-model.html

为了实现 a large-scale, busy, or highly reliable database application, 又或为了fine tune MySQL的性能, 你必须要理解MySQL InnoBD locking and the InnoDB transaction model.

1. [InnoDB Locking](innodb_locking.md)
2. [InnoDB Transaction Model](#innodb-transaction-model)
3. Locks Set by Different SQL Statements in InnoDB
4. Phantom Rows
5. Deadlocks in InnoDB
6. Transaction Scheduling

### InnoDB Transaction Model

1. [Transaction Isolation Level](isolation_level.md)
2. [Autocommit, Commit, and Rollback](autocommit_commit_rollback.md)
3. Consistent Nonlocking Reads
4. [Locking Reads](locking_read.md)
