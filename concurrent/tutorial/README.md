# Concurrency

http://web.mit.edu/6.005/www/fa15/classes/19-concurrency/

# Thread Safe

http://web.mit.edu/6.005/www/fa15/classes/20-thread-safety/

There are basically four ways to make variable access safe in shared-memory concurrency:

- Confinement
- Immutability
- Thread safe type
- Synchronization

# Locks and Synchronization

http://web.mit.edu/6.005/www/fa15/classes/23-locks/#concurrency_in_practice

**Locks are one synchronization technique**. A lock is an abstraction that allows at most one thread to own it at a time. Holding a lock is how one thread tells other threads: “I’m changing this thing, don’t touch it right now.”
