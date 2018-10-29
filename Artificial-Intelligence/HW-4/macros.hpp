#pragma once

// define maze scale
#define ROW 3
#define COL 4

// define maze node state
#define AVAILABLE 1
#define WALL -1
#define EXIT 0

// never used
#define N 0
#define E 1
#define S 2
#define W 3
#define REVERSE(x)  ((x + 2) % 4)
