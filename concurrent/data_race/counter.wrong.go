package main

import (
	"fmt"
	"sync"
)

type count32 int32

func (c *count32) inc() int32 {
	//return atomic.AddInt32((*int32)(c), 1)
	*c++
	return int32(*c)
}

func (c *count32) get() int32 {
	//return atomic.LoadInt32((*int32)(c))
	return int32(*c)
}

var c count32

func main() {
    loop := 1000
    fmt.Printf("start %d goroutine concurrent incr counter:\n", loop)
	wg := sync.WaitGroup{}
	for i := 0; i < loop; i++ {
		wg.Add(1)
		go func() {
			c.inc()
			wg.Done()
		}()
	}
	wg.Wait()
	fmt.Printf("Got:\t%d\n", c.get())
}

