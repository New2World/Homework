#include <iostream>

#include "pancake.hpp"
#include "heuristic.hpp"

using namespace std;

int main(){
    string start;
    cin >> start;
    pancake startState(start);
    pancake endState("4321");
    cout << endl;

    Heuristic<pancake> astar;

    vector<pancake> path;

    astar.search(startState, endState, path);

    for(int i = 0;i < path.size();i++)
        cout << path[i] << endl;

    return 0;
}
