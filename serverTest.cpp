#include<iostream>
#include<thread>

#include"lock-free-stack.h"

LFStack<int> stack; 
bool start;

void pusher(int i) {
    
}

void poper() {

}

void searcher() {

}

int main() {
    std::vector<std::thread> threads;
    for(int i = 0; i < 100; ++i) {
        if(i % 100 == 0) {
            threads.push_back(std::thread(poper));
        } else if(i % 10 == 0) {
            threads.push_back(std::thread(pusher, i / 10));
        } else {
            threads.push_back(std::thread(searcher));
        }
    }

    for(auto& th : threads) {
        th.join();
    }
    return 0;
}
