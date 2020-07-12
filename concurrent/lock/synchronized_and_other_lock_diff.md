# ReentrantLock Example in Java, Difference between synchronized vs ReentrantLock

ref: https://javarevisited.blogspot.com/2013/03/reentrantlock-example-in-java-synchronized-difference-vs-lock.html

1. use lock 获取锁可打断, specify timeout for "acquire" lock, `synchronized` cann't

2. fairness, use lock 可以被指定为"公平锁", 等待时间最长的线程优先获取锁, `synchronized` cann't

3. `tryLock()`, use lock can "test" can acquire lock or not, to reduce wait lock time, `synchronized` cann't

4. the ability to interrupt, use lock 可以在线程等待锁期间被"中断"(通过`lockInterruptibly()`方法), `synchronized` cann't

5. get List of all threads waiting for lock, use lock you can do this;  `synchronized` cann't

Lock interface adds lot of power and flexibility and allows some control over lock acquisition process, which can be leveraged to write highly scalable systems in Java.

