#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <functional>

class pancake{
    std::string str;
    int cost;
    int h() const {
        for(int i = 0;i < 4;i++)
            if(str[i] - '0' != 4 - i)
                return 4 - i;
        return 0;
    }
public:
    pancake(){str = "4321", cost = 0;}
    pancake(const pancake& p){str = p.str, cost = p.cost;}
    pancake(std::string str, int cost=0):str(str),cost(cost){}
    std::vector<pancake> run(std::vector<pancake>&);

    inline std::string getStr() const {
        return str;
    }

    bool operator < (const pancake p) const {
        int f = cost + h();
        int f_p = p.cost + p.h();
        if(f == f_p)
            return str < p.str;
        return f > f_p;
    }

    inline bool operator == (const pancake p) const {
        return str == p.str;
    }

    inline bool operator != (const pancake p) const {
        return str != p.str;
    }

    friend std::ostream& operator << (std::ostream& outp, pancake& p) {
        outp << p.getStr() << " cost: " << p.cost;
        return outp;
    }
};

std::vector<pancake> pancake::run(std::vector<pancake>& next){
    std::string temp;
    for(int i = 0;i <= 2;i++){
        temp = str;
        std::reverse(temp.begin()+i, temp.end());
        next.push_back(pancake(temp,cost + 4 - i));
    }
    return next;
}

namespace std{
    template<>
    struct hash<pancake>{
        size_t operator () (const pancake& p) const {
            return hash<string>()(p.getStr());
        }
    };
}
