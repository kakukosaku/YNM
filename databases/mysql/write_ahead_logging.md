# Write-Ahead Logging, WAL

Ref: https://www.postgresql.org/docs/9.1/wal-intro.html

Briefly, WAL's central concept is that changes to data files (where tables and indexes reside) must be written only after those changes have been logged, that is, after log records describing the changes have been flushed to permanent storage.
