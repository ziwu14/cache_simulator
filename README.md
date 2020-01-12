#### Introduction

A cache simulator that could run cache miss test on configurable caches. Cache properties include

> * number of layers
> * cache type
> * capacity
> * block size
> * associativity
> * Write allocate
> * Write back or write through
> * replacement policy

The test will not only report miss rate but also types of misses

* compulsory misses
* capacity misses
* conflict misses

#### Usage

```bash
$ make
$ ./myProgram
```

