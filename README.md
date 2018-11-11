# a userland preemption test using ptrace

## requirements
- docker
- make

## how to test it
```
$ make docker
$ make run
```

`make run` will run the test program for 30 seconds.

## files
target program
- test.cc (main routine)
- int.S (handler)

context dispatcher
- dispatcher.cc

