# a userland preemption test using ptrace

## what's this?
`dispatcher` calls `handler` every 3 seconds, and `handler` switches worker contexts.

## requirements
- docker
- make

## how to test it
```
$ make docker
$ make run
```

`make run` will run the worker program for 15 seconds.

## files
target program
- worker.cc (workers)
- int.S (handler)

context dispatcher
- dispatcher.cc

