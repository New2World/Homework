#include "tools.hpp"

using namespace std;

int main(int argc, char** argv){
    int maxIter = 10000;
    vector<int> inputList;
    if(argc <= 2){
        if(argc == 2)
            maxIter = atoi(argv[1]);
        inputList = parseInput();
    }
    else{
        inputList = parseArgv(argv);
        argc -= inputList.size();
        if(argc > 2)
            maxIter = atoi(argv[inputList.size() + 2]);
    }
    Maze maze(inputList);
    QLearning agent(.1,.5,.1,maxIter);
    agent.train(maze);
    maze.output();
    return 0;
}
