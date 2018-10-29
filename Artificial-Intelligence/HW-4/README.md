# Homework 4

## Introduction

### Files

- `macros.hpp`: some definitions of macros;  
- `tools.hpp`: definitions of some functions and two important classes;  
- `hw4.cpp`: entry of the program.

### Classes

- `Maze`: class to store and maintain information of the grid world;  
- `QLearning`: class to perform Q-learning algorithm step by step in the grid world defined by class `Maze`.

## How to Run

### Compile

```bash
$ g++ --std=c++11 -O3 hw4.cpp -o hw4.out
```

### Run

```bash
$ ./hw4.out [args] [iters]
```

- args: input of the homework, like `12 8 6 p` or `12 8 6 q 11`. This term can be ignored, if so you should input during the run time;  
- iters: maximum iterations the algorithm should run, you can use this argument to specify maximum iteration without adding `args`. If leaving this argument as default, the algorithm will halt after 10,000 iterations;  
