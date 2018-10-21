/****************************************************************************/
/*																			*/
/* author: Ruiqi Wang														*/
/* date: Sep 20, 11:43														*/
/*																			*/
/* Since there must be a solution,											*/
/* the program ignores cases where solution is not available.				*/
/*																			*/
/****************************************************************************/

#include <iostream>
#include <string>
#include <map>
#include <set>
#include <stack>
#include <queue>
#include <algorithm>

#define GOAL "4321"

using namespace std;

// closed set, store states that have been visited once
set<string> closed;
// store heuristic value of each state
map<string, int> heuristic;
// record previous state of each state in the path
map<string, string> prevState;

void preloadHeuristic() {
	// pre-load all heuristic values into a array
	string stringSeq = GOAL;
	do {
		heuristic[stringSeq] = 0;
		for (int i = 0; i < 4; i++) {
			if (stringSeq[i] - '0' != 4 - i) {
				heuristic[stringSeq] = 4 - i;
				break;
			}
		}
	} while (prev_permutation(stringSeq.begin(), stringSeq.end()));
}

string flip(string str, int pos) {
	// flip pancake from the given position
	// str: pancake
	// pos: the number of pancakes to be flip on the top
	if (pos > (int)str.size())
		return str;
	reverse(str.end() - pos, str.end());
	return str;
}

int getFlipPosition(string prev, string str) {
	// infer the number of pancakes that are fliped from both previous and current pancake state
	// prev: previous pancake state
	// str: current pancake state
	for (int i = 0; i < 4; i++)
		if (prev[i] != str[i])
			return 4 - i;
	return 0;
}

void output(string pc, int pos, int cost, bool heur=true) {
	// format output
	// pc: pancake
	// pos: where should the '|' be placed
	// cost: total cost from start state to current state
	// heur: if heuristic values are required
	int h = 0;
	if (heur)
		h = heuristic[pc];
	if(pos >= 0)
		pc.insert(pc.end() - pos, '|');
	cout << pc;
	if (cost >= 0)
		cout << "\tg = " << cost << ", h = " << h;
	cout << endl;
}

int outputRecursively(string start, int cost, bool heur=true) {
	// the order we get from UCS and A* is from goal to start state
	// this recursion reverse the output order
	if (prevState[start].size() <= 0)
		return cost;
	cost = outputRecursively(prevState[start], cost);
	int currentCost = getFlipPosition(prevState[start], start);
	output(prevState[start], currentCost, cost, heur);
	if(cost >= 0)
		return cost + currentCost;
	return cost;
}

// DFS Recursion
//bool DFSstep(string start, vector<int>& path) {
//	string next;
//	if (start == GOAL)
//		return true;
//	closed.insert(start);
//	priority_queue<string> nextPancake;
//	for (int i = 2; i <= 4; i++)
//		nextPancake.push(flip(start, i));
//	while(!nextPancake.empty()) {
//		next = nextPancake.top();
//		nextPancake.pop();
//		if (closed.find(next) != closed.end())
//			continue;
//		if (DFSstep(next, path)) {
//			path.push_back(getFlipPosition(start, next));
//			return true;
//		}
//	}
//	return false;
//}
//
//void DFS_recursive(string start) {
//	vector<int> path;
//	DFSstep(start, path);
//	for (int i = path.size()-1; i >= 0; i--) {
//		output(start, path[i], -1);
//		start = flip(start, path[i]);
//	}
//	output(start, -1, -1);
//}

void DFS(string start) {
	// fringe is a stack
	// remove current state from the fringe, meanwhile add all possible states which current state can reach into the fringe
	// states with larger lexicographical order come out of fringe first
    int cost = 0;
	stack<string> fringe;
	// use priority_queue to keep lexicographical order
	priority_queue<string, vector<string>, greater<string> > pq;

	// start the fringe with new state from the start state
	for (int i = 2; i <= 4; i++)
		fringe.push(flip(start, i));
	while (!fringe.empty()) {
		if (start == GOAL)
			break;
		for (int i = 0; i < 3; i++) {
			pq.push(fringe.top());
			fringe.pop();
		}
		// sort the new state with lexicographical order using priority_queue
		while (!pq.empty()) {
			fringe.push(pq.top());
			pq.pop();
		}
        while(closed.find(fringe.top()) != closed.end())
            fringe.pop();
		output(start, getFlipPosition(start, fringe.top()), cost);
        cost += getFlipPosition(start, fringe.top());
		start = fringe.top();
		fringe.pop();
        while(!fringe.empty())
            fringe.pop();
        closed.insert(start);
        // add upcoming states which can be reached from current state to fringe
		for (int i = 2; i <= 4; i++)
			fringe.push(flip(start, i));
	}
	output(start, -1, cost);
}

void UCS(string start) {
	// fringe is a priority queue
	// remove current state from the fringe, meanwhile add all possible states which current state can reach into the fringe
	// states with smaller cost and larger lexicographical order come out of fringe first

	struct stateNode {
		// define a structure to store both previous and current pancake state and cost of current state
		// cost: total flips from start state to current state
		// prev: previous pancake state
		// pancake: current pancake state
		int cost;
		string prev, pancake;
		stateNode(string pancake, string prev = "", int cost = 0) :cost(cost), prev(prev), pancake(pancake) {}
	};

	// Uniform-Cost Search only care about the cost g(n)
	auto cmp = [](stateNode node1, stateNode node2)-> bool {
		// compare function: state with smaller cost and larger lexicographical order is preferred.
		if (node1.cost == node2.cost)
			return node1.pancake < node2.pancake;
		return node1.cost > node2.cost;
	};
	priority_queue<stateNode, vector<stateNode>, decltype(cmp)> fringe(cmp);

	vector<pair<int, string> > path;
	stateNode node = stateNode(start);
	fringe.push(node);
	while (!fringe.empty()) {
        // expand the node with smallest cost
		node = fringe.top();
		fringe.pop();
		// avoid visiting twice
		if (closed.find(node.pancake) != closed.end())
			continue;
		// add current state to closed set
		closed.insert(node.pancake);
		// store the previous state in order to find the final path
		prevState[node.pancake] = node.prev;
		if (node.pancake == GOAL)
			break;
		// add upcoming states which can be reached from current state to fringe
		for (int i = 2; i <= 4; i++)
			fringe.push(stateNode(flip(node.pancake, i), node.pancake, node.cost + i));
	}
	// output recursively from back to the front
	output(node.pancake, -1, outputRecursively(node.pancake, 0, false));
}

void Greedy(string start) {
	// fringe is a priority queue
	// remove current state from the fringe, meanwhile add all possible states which current state can reach into the fringe
	// states with smaller heuristic and larger lexicographical order come out of fringe first

	struct stateNode {
		// define a structure to store both previous and current pancake state and cost of current state
		// cost: total flips from start state to current state
		// prev: previous pancake state
		// pancake: current pancake state
		int cost;
		string prev, pancake;
		stateNode(string pancake, string prev="", int cost=0) :cost(cost), prev(prev), pancake(pancake) {}
	};

	// f(n) = g(n) + h(n)
	auto cmp = [](stateNode node1, stateNode node2)-> bool {
		// compare function: state with smaller f (= cost + heuristic) and larger lexicographical order is preferred.
		if (heuristic[node1.pancake] == heuristic[node2.pancake])
			return node1.pancake < node2.pancake;
		return heuristic[node1.pancake] > heuristic[node2.pancake];
	};
	priority_queue<stateNode, vector<stateNode>, decltype(cmp)> fringe(cmp);

	vector<pair<int, string> > path;
	stateNode node = stateNode(start);
	fringe.push(node);
	while (!fringe.empty()) {
        // expand the node with smallest f = cost + heuristic value
		node = fringe.top();
		fringe.pop();
		// avoid visiting twice
		if (closed.find(node.pancake) != closed.end())
			continue;
		// add current state to closed set
		closed.insert(node.pancake);
		// store the previous state in order to find the final path
		prevState[node.pancake] = node.prev;
		if (node.pancake == GOAL)
			break;
		// add upcoming states which can be reached from current state to fringe
		for(int i = 2;i <= 4;i++)
			fringe.push(stateNode(flip(node.pancake, i), node.pancake, node.cost + i));
	}
	// output recursively from back to the front
	output(node.pancake, -1, outputRecursively(node.pancake, 0));
}

void AStar(string start) {
	// fringe is a priority queue
	// remove current state from the fringe, meanwhile add all possible states which current state can reach into the fringe
	// states with smaller f (= cost + heuristic) and larger lexicographical order come out of fringe first

	struct stateNode {
		// define a structure to store both previous and current pancake state and cost of current state
		// cost: total flips from start state to current state
		// prev: previous pancake state
		// pancake: current pancake state
		int cost;
		string prev, pancake;
		stateNode(string pancake, string prev="", int cost=0) :cost(cost), prev(prev), pancake(pancake) {}
	};

	// f(n) = g(n) + h(n)
	auto cmp = [](stateNode node1, stateNode node2)-> bool {
		// compare function: state with smaller f (= cost + heuristic) and larger lexicographical order is preferred.
		int f1 = node1.cost + heuristic[node1.pancake];
		int f2 = node2.cost + heuristic[node2.pancake];
		if (f1 == f2)
			return node1.pancake < node2.pancake;
		return f1 > f2;
	};
	priority_queue<stateNode, vector<stateNode>, decltype(cmp)> fringe(cmp);

	vector<pair<int, string> > path;
	stateNode node = stateNode(start);
	fringe.push(node);
	while (!fringe.empty()) {
        // expand the node with smallest f = cost + heuristic value
		node = fringe.top();
		fringe.pop();
		// avoid visiting twice
		if (closed.find(node.pancake) != closed.end())
			continue;
		// add current state to closed set
		closed.insert(node.pancake);
		// store the previous state in order to find the final path
		prevState[node.pancake] = node.prev;
		if (node.pancake == GOAL)
			break;
		// add upcoming states which can be reached from current state to fringe
		for(int i = 2;i <= 4;i++)
			fringe.push(stateNode(flip(node.pancake, i), node.pancake, node.cost + i));
	}
	// output recursively from back to the front
	output(node.pancake, -1, outputRecursively(node.pancake, 0));
}

int main() {
	string startState;
	cin >> startState;
	cout << endl;

	string start(startState.begin(), --startState.end());
	switch (startState[4]) {
	case 'd':
		DFS(start);
		break;
	case 'u':
		UCS(start);
		break;
	case 'g':
		preloadHeuristic();
		Greedy(start);
		break;
	case 'a':
		preloadHeuristic();
		AStar(start);
        break;
	}

	return 0;
}
