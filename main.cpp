#include<iostream>
#include<thread>

#include"lock-free-stack.h"

LFStack<int> stack;
bool start = false;

void fun(int num) {
    ThreadEpoch<int> epoch;
    while(!start);
    for(int i = 0; i < 5; ++i) {
        stack.push(num, epoch);
    }
}

int main() {
    std::vector<std::thread> array;
    for(int i = 0; i < 100; ++i) {
        array.push_back(std::thread(fun, i + 1));
    }
    ThreadEpoch<int> epoch;
    start = true;
    
    stack.search(5, epoch);

    for(auto& thr : array) {
        thr.join();
    }

    /*
    stack.for_each([](int i) {
        std::cout << i << ' ';
    });
    std::cout << std::endl;
    */
    while(!stack.empty()) {
        auto ptr = stack.pop(epoch);
        std::cout << *ptr << ' ';
    }
    std::cout << std::endl;
    
    return 0;    
}
