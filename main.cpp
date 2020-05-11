#include<iostream>
#include<thread>

#include"lock-free-stack.h"

LFStack<int> stack;
bool start = false;

void fun(int num) {
    while(!start);
    ThreadEpoch<int> epoch;
    for(int i = 0; i < 5; ++i) {
        stack.push(num, epoch);
        //usleep(50);
    }
}

int main() {
    std::vector<std::thread> array;
    for(int i = 0; i < 100; ++i) {
        array.push_back(std::thread(fun, i + 1));
    }

    start = true;
    ThreadEpoch<int> epoch;
    
    for(auto& thr : array) {
        thr.join();
    }
    std::cout << "In stack: ";
    stack.for_each([](int i) {
        std::cout << i << ' ';
    });
    std::cout << std::endl;
    
    std::cout << "Poped: ";
    while(!stack.empty()) {
        auto ptr = stack.pop(epoch);
        std::cout << *ptr << ' ';
    }
    std::cout << std::endl;
    
    return 0;    
}
