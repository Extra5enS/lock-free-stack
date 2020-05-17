#include<iostream>
#include<thread>

#include"lock-free-stack.h"

LFStack<int> stack; 
bool start;

void pusher(int i) {
    ThreadEpoch<int> epoch;
    while(!start);
    for(int j = 0; j < 100; ++j) {
        stack.push(i, epoch);
    }
}

void poper() {
    ThreadEpoch<int> epoch;
    while(!start);
    for(int i = 0; i < 100; ++i) {
        *stack.pop(epoch);
    }
}

void searcher(int i) {
    ThreadEpoch<int> epoch;
    while(!start);
    for(int j = 0; j < 100; ++j) {
        stack.search(i, epoch);
    }
}

int main(int argc, char** argv) {
    if(argc != 2) {
        fprintf(stderr, "Args error!\n");
        return -1;
    } 
    int threadN = atoi(argv[1]);
    if(threadN / 100 == 0 || threadN % 100 != 0) {
        fprintf(stderr, "I may have problems with such args!\n");
    }
    double secCount = 0;
    for(int k = 0; k < 20; ++k) {
        std::vector<std::thread> threads;
        for(int i = 0; i < threadN; ++i) {
            if(i % 100 == 0) {
                threads.push_back(std::thread(poper));
            } else if(i % 10 == 0) {
                threads.push_back(std::thread(pusher, i / 10));
            } else {
                threads.push_back(std::thread(searcher, i / 10));
            }
        }

        auto init = std::chrono::steady_clock::now();
        start = true;
        for(auto& th : threads) {
            th.join();
        }
        std::chrono::duration<double> elapsed_seconds; 
        elapsed_seconds = std::chrono::steady_clock::now() - init; 
        secCount += elapsed_seconds.count(); 
    }
    std::cout << threadN << " " << secCount / 20 << std::endl;

    return 0;
}
