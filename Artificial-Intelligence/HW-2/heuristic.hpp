#pragma once

#include <queue>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>

template<typename _NODE_TYPE>
class Heuristic{
private:
    std::unordered_set<_NODE_TYPE> closed;
    std::unordered_map<_NODE_TYPE, _NODE_TYPE> previous;
    std::priority_queue<_NODE_TYPE> fringe;

    void reconstructPath(_NODE_TYPE, _NODE_TYPE, std::vector<_NODE_TYPE>&);
public:
    Heuristic(){};
    void search(_NODE_TYPE, _NODE_TYPE, std::vector<_NODE_TYPE>&);
};

template<typename _NODE_TYPE>
void Heuristic<_NODE_TYPE>::reconstructPath(_NODE_TYPE start, _NODE_TYPE end, std::vector<_NODE_TYPE>& path){
    _NODE_TYPE node = end;
    while(node != start){
        path.push_back(node);
        node = previous[node];
    }
    path.push_back(node);
    reverse(path.begin(), path.end());
}

template<typename _NODE_TYPE>
void Heuristic<_NODE_TYPE>::search(_NODE_TYPE start, _NODE_TYPE end, std::vector<_NODE_TYPE>& path){
    std::vector<_NODE_TYPE> nextNodes;
    fringe.push(start);
    _NODE_TYPE node = start;
    while(!fringe.empty()){
        node = fringe.top();
        fringe.pop();
        if(closed.find(node) != closed.end())
            continue;
        closed.insert(node);
        nextNodes.clear();
        node.run(nextNodes);
        if(node == end)
            break;
        for(int i = 0;i < nextNodes.size();i++){
            if(closed.find(nextNodes[i]) != closed.end())
                continue;
            fringe.push(nextNodes[i]);
            previous[nextNodes[i]] = node;
        }
    }
    reconstructPath(start, node, path);
}
