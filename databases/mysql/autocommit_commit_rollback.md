# autocommit, Commit, and Rollback

Ref:

1. https://dev.mysql.com/doc/refman/8.0/en/innodb-autocommit-commit-rollback.html

In innoDB, all user activity occurs inside a transaction.

If [autocommit](https://dev.mysql.com/doc/refman/8.0/en/server-system-variables.html#sysvar_autocommit) mode is enabled, **each SQL statement forms a single transaction on its own.**

By default, MySQL starts the session for each new connection with autocommit enabled, so MySQL does a commit after each SQL statement did not return an error. if a statement returns an error, the commit or rollback behavior depends on the error. See [Section 15.21.4 InnoDB Error Handling](https://dev.mysql.com/doc/refman/8.0/en/innodb-error-handling.html)

If autocommit mode is disabled within a session with `SET autocommit = 0`, the session always has a transaction open. A COMMIT or ROLLBACK statement ends the current transaction and a new one starts.

If a session that has autocommit disabled ends without explicitly committing the final transaction, MySQL rolls back that transaction.

Some statements implicitly end a transaction, as if you had done a COMMIT before executing the statement.

A COMMIT means that the changes made in the current transaction are made permanent and become visible to other sessions.

A ROLLBACK statement, on the other hand, cancels all modifications made by the current transaction.

Both COMMIT and ROLLBACK release all InnoDB locks that were set during the current transaction.
