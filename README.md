# Parallel Programming Project 1


For compile all code:

```bash

    make all

```

This will compile all .c file to ./bin folder.

You can run seq_main or mpi_main that is paralel of seq_main with :
```bash

    make runSequential1
    make runParallel2


```

You can change process count when calling definition of function.

For example:

This will run mpi_man with 4 process.

```bash


    make runParallel4


```