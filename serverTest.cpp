#include<iostream>
#include<thread>

#include"lock-free-stack.h"

LFStack<int> stack; 
bool start;
std::atomic<int> finish = {0};

void pusher(int i) {
    ThreadEpoch<int> epoch;
    while(!start);
    for(int j = 0; j < 100; ++j) {
        stack.push(i, epoch);
    }
    finish++;
}

void poper() {
    ThreadEpoch<int> epoch;
    while(!start);
    for(;!stack.empty();) {
        std::shared_ptr<int> buf;
        if((buf = stack.pop(epoch)) != nullptr) {
            //fprintf(stderr, "%d ", *buf);
        }
    }
    finish++;
}

void searcher(int i) {
    ThreadEpoch<int> epoch;
    while(!start);
    for(int j = 0; j < 100; ++j) {
        stack.search(i, epoch);
    }
    finish++;
}

int main(int argc, char** argv) {
    if(argc != 2) {
        fprintf(stderr, "Args error!\n");
        return -1;
    } 
    int threadN = atoi(argv[1]);
    double secCount = 0;
    finish.store(0);
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
    start = true; // start work;

    while(finish.load() < threadN); 

    std::chrono::duration<double> elapsed_seconds; 
    elapsed_seconds = std::chrono::steady_clock::now() - init; 
    secCount += elapsed_seconds.count(); 

    for(auto& th : threads) {
        th.join();
    }
    std::cout << threadN << " " << secCount << std::endl;
    return 0;
}
