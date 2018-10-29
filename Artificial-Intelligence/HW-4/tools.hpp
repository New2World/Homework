#pragma once

#include <iostream>
#include <iomanip>
#include <utility>
#include <vector>
#include <cmath>

#include <cstdlib>

#include "macros.hpp"

// transform index to coordination
std::pair<int,int> id2xy(int index){
    if(index > 12)
    return id2xy(12);
    if(index < 1)
    return id2xy(1);
    int row = 3 - (index - 1) / 4 - 1;
    int col = (index - 1) % 4;
    return std::make_pair(row, col);
}

// split input string and convert to integers
std::vector<int> parseInput(){
    int temp;
    char ch[2];
    std::vector<int> inputList;
    std::cout << "Input: ";
    std::cout.flush();
    for(int i = 0;i < 3;i++){
        std::cin >> temp;
        inputList.push_back(temp);
    }
    scanf("%s", ch);
    if(ch[0] == 'q'){
        std::cin >> temp;
        inputList.push_back(temp);
    }
    return inputList;
}

// parse arguments
std::vector<int> parseArgv(char **argv){
    std::vector<int> inputList;
    for(int i = 1;i <= 3;i++)
        inputList.push_back(atoi(argv[i]));
    if(argv[4][0] == 'q')
        inputList.push_back(atoi(argv[5]));
    return inputList;
}

// transform coordination to index
int xy2id(std::pair<int,int> pos){
    return (3 - pos.first - 1) * COL + pos.second + 1;
}
int xy2id(int x, int y){
    return (3 - x - 1) * COL + y + 1;
}

// define directions {north, east, south, west} and corresponding char labels
int direct[4][2] = {{-1,0},{0,1},{1,0},{0,-1}};
char direct_ch[4] = {'^', '>', 'v', '<'};

class Maze{
    std::pair<int,int> position, prevPosition;
    std::pair<int,int> donut, forbidden;

    // return the next position after taking action i
    std::pair<int,int> takeStep(int i){
        std::pair<int,int> pos = position;
        pos.first += direct[i][0];
        pos.second += direct[i][1];
        return pos;
    }
public:
    typedef struct{
        int state;
        float reward;
        float qval[4];

        // return the policy agent will take at current position
        int policy(){
            int policy = 0;
            for(int i = 1;i < 4;i++)
                if(qval[i] > qval[policy])
                    policy = i;
            return policy;
        }
    } _MazeNode;
    typedef _MazeNode _Maze[ROW][COL];

    int queryPosition = 0;
    _Maze maze;

    // initialize
    Maze(std::vector<int> inputList){
        reset();
        for(int i = 0;i < ROW;i++)
            for(int j = 0;j < COL;j++)
                maze[i][j] = {AVAILABLE, -.1, {0.,0.,0.,0.}};
        std::pair<int,int> pos;
        pos = id2xy(inputList[0]);
        maze[pos.first][pos.second] = {EXIT, -.1,{0.,0.,0.,0.}};
        donut = pos;
        pos = id2xy(inputList[1]);
        maze[pos.first][pos.second] = {EXIT, -.1,{0.,0.,0.,0.}};
        forbidden = pos;
        pos = id2xy(inputList[2]);
        maze[pos.first][pos.second] = {WALL, 0.,{0.,0.,0.,0.}};
        if(inputList.size() > 3)
            queryPosition = inputList[3];
    }

    _MazeNode& current(){
        return maze[position.first][position.second];
    }

    // update q-value for previous state
    void update(int act, float lr, float val){
        maze[prevPosition.first][prevPosition.second].qval[act] *= 1 - lr;
        maze[prevPosition.first][prevPosition.second].qval[act] += val * lr;
    }

    // update q-value for the exit point
    void updateExit(float lr){
        maze[position.first][position.second].qval[0] *= (1 - lr);
        maze[position.first][position.second].qval[0] += lr * exitReward();
    }

    // judge if position p is in the maze
    bool isValid(std::pair<int,int> p){
        return p.first < ROW && p.second < COL && p.first >= 0 && p.second >= 0;
    }

    // judge if position p is reachable
    bool isAvailable(std::pair<int,int> p){
        return isValid(p) && maze[p.first][p.second].state >= 0;
    }

    bool isAvailable(int i){
        std::pair<int,int> p = takeStep(i);
        return isAvailable(p);
    }

    // judge if the point goes to a donut or forbidden point
    bool isExit(std::pair<int,int> p){
        return maze[p.first][p.second].state == EXIT;
    }

    bool isExit(){
        return maze[position.first][position.second].state == EXIT;
    }

    // restart the maze game by set state to the first position
    void reset(){
        position = id2xy(1);
        prevPosition = position;
    }

    // take a move according to the action
    float takeAction(int act){
        std::pair<int,int> pos = takeStep(act);
        prevPosition = position;
        if(isAvailable(pos))
            position = pos;
        return maze[position.first][position.second].reward;
    }

    // choose a random action in epsilon-greedy
    float randomAction(){
        return rand() % 4;
    }

    // return the reward for going to donut or forbidden point
    float exitReward(){
        if(position == donut)
            return 100.;
        return -100.;
    }

    // output the maze
    void output(){
        std::pair<int,int> pos;
        if(queryPosition > 0){
            if(queryPosition > 12){
                std::cerr << "index out of range" << std::endl;
                return;
            }
            for(int i = 0;i < 4;i++){
                pos = id2xy(queryPosition);
                std::cout << direct_ch[i] << "  "
                          << maze[pos.first][pos.second].qval[i] << std::endl;
            }
        }
        else{
            for(int i = 1;i <= ROW * COL;i++){
                pos = id2xy(i);
                if(!isAvailable(pos) || isExit(pos))
                    continue;
                std::cout << i << "  "
                          << direct_ch[maze[pos.first][pos.second].policy()] << std::endl;
            }
        }
    }
};

class QLearning{
    float lr, discount, explore;
    int maxIter;

    bool exploit();

    // set the character width in output
    int fixedWidth(){
        return (int)log10f((float)maxIter) + 1;
    }

    // decrease epsilon while running learning process
    void exploreDecrease(int iter){
        static int lastDecrease = 0;
        if(iter - 1 >= (maxIter + lastDecrease) / 2){
            lastDecrease = iter;
            explore *= .5;
        }
    }
public:
    QLearning(){}
    QLearning(const QLearning&){}
    QLearning(float, float, float, int=10000);

    void train(Maze&);
};

QLearning::QLearning(float lr, float discount, float explore, int maxIter):
lr(lr), discount(discount), explore(explore), maxIter(maxIter){
    srand(time(NULL));
}

// exploitation vs exploration
bool QLearning::exploit(){
    float prob = 1. * rand() / RAND_MAX;
    if(prob < explore)
        return false;
    return true;
}

// perform q learning and get q values
void QLearning::train(Maze& maze){
    std::cout << std::endl << "training with maximum " << maxIter  << " iterations..." << std::endl;
    int iterCount = 1;
    int action = -1;
    float reward = 0.;
    while(iterCount - 1 < maxIter){
        if(!exploit())
            action = maze.randomAction();
        else{
            action = maze.current().policy();
        }
        reward = maze.takeAction(action);
        reward += discount * maze.current().qval[maze.current().policy()];
        maze.update(action, lr, reward);
        exploreDecrease(iterCount);
        if(maze.isExit()){
            maze.updateExit(lr);
            maze.reset();
            iterCount++;
        }
    }
    std::cout << std::endl;
}
