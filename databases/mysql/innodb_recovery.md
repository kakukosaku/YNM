# InnoDB Recovery

## Overview

Ref:

1. https://dev.mysql.com/doc/refman/8.0/en/innodb-recovery.html
2. https://dev.mysql.com/doc/refman/8.0/en/point-in-time-recovery.html
3. http://www.tocker.ca/2013/05/06/when-does-mysql-perform-io.html

- Point-in-Time Recovery
- Recovery from Datq Corruption or Disk Failure
- InnoDB Crash Recovery
- Tablespace Discovery During Crash Recovery

### InnoDB Crash Recovery

1. Tablespace discovery
2. Redo log application
3. Roll back of incomplete transaction
4. change buffer merge
5. Purge
