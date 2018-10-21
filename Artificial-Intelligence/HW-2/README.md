## Simple Heuristic Search Framework (for practice)

This is somehow a simple framework in cpp allowing user to do some simple heuristic search. And the strategy of the fringe operation is decided by user related to the way they do heuristic.  

### How to use

It is implemented as a template class, so states are data structures defined by users. However, due to the implement, way to define state structure can be a little tricky.

The state structure mush overload operator `==` `!=` and `<`. Besides, to use the features of `unordered_map` and `unordered_set`, `struct hash<state_structure>` must be specialized in name space `std`. (example: `pancake.hpp`)

### What can it do

Since this is something I do to extend course project, there's only one function aiming at finding the optimal solution to some search problems using heuristic algorithms. In the demo, it's an A* algorithm, regarding `f(n) = g(n) + h(n)` as heuristic.
