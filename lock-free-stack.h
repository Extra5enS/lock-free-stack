#pragma once

#include<mutex>
#include<atomic>
#include<unistd.h>
#include<functional>
#include<memory>
#include<vector>


#define SLEEP_TIME 10
#define EPOCH_COUNT 3

template<class T>
struct Node {
    T value;
    Node* next;
    Node(T const& value): value(value), next(nullptr) {}
    ~Node() {}
};

template<class T>
std::vector<std::atomic<Node<T>*>> array(EPOCH_COUNT); // We well work with it like stacks with only one deleter. 
std::atomic<unsigned> globalEpoch = {1};
std::atomic<unsigned> threadInEpoch = {0};
std::atomic<unsigned> threadCount = {0};
std::atomic<bool> inDelete = {false}; 

template<class T>
struct ThreadEpoch {
    unsigned threadEpoch;
    int whoIam = -1;
    ThreadEpoch(): threadEpoch(1), whoIam(whoIam) {
        threadCount++;   
    }

    auto add(Node<T>* ptr) -> void { // add like push
        auto& head = array<T>[threadEpoch % EPOCH_COUNT];
        ptr -> next = head.load();
        //fprintf(stderr, "%p\n", array<T>[threadEpoch % EPOCH_COUNT].load());
                 
        while(!head.compare_exchange_weak(ptr -> next, ptr)) {
            usleep(SLEEP_TIME);
        }
        //fprintf(stderr, "%p -> %p\n", array<T>[threadEpoch % EPOCH_COUNT].load(), 
        //        array<T>[threadEpoch % EPOCH_COUNT].load() -> next);
    }

    auto enter() -> void {
        if(threadEpoch <= globalEpoch.load()) {
            //fprintf(stderr, "%d\n", threadEpoch);
            threadEpoch = globalEpoch.load(std::memory_order_release) + 1u;
            threadInEpoch++;
        }
    }

    auto exit() -> void {
        auto thc = threadCount.load();
        auto thie = threadInEpoch.load();
        /*
        if(thc < thie) {
            threadInEpoch.store(thc);
        }*/

        bool f = false;
        if(inDelete.compare_exchange_weak(f, true)) {
            
            if(threadInEpoch.compare_exchange_weak(thc, 0)) {
                globalEpoch++;
                int iter = (globalEpoch.load() - 1) % EPOCH_COUNT;
                for(Node<T>* start = array<T>[iter].load(std::memory_order_release); start != nullptr;) {
                    auto p = start;
                    start = start -> next;
                    delete p;
                }
                array<T>[iter] = nullptr;
            }
            inDelete.store(false);
        }
    }

    ~ThreadEpoch() {
        threadInEpoch--;
        threadCount--;
    }
};


    template<class T>
class LFStack {
    std::mutex iterMutex;
    std::atomic<Node<T>*> head;
    std::atomic<unsigned> threadInStack; 

    public:
    LFStack() {}
    auto empty() -> bool {
        return head.load() == nullptr;
    }

    auto push(T const& value, ThreadEpoch<T>& epoch) -> void {
        //try to come in 
        iterMutex.lock(); //only for lock in LFStack::for_each(...);
        iterMutex.unlock();        
        threadInStack++;
        epoch.enter();

        Node<T>* const newNode = new Node<T>(value);
        newNode -> next = head.load();
        while(!head.compare_exchange_weak(newNode -> next, newNode)) { 
            newNode -> next = head.load();
            usleep(SLEEP_TIME);
        }

        epoch.exit();

        threadInStack--;
    }

    auto search(T value, ThreadEpoch<T>& epoch) -> bool {
        // try to come inA
        iterMutex.lock(); //only for lock in LFStack::for_each(...);
        iterMutex.unlock();
        threadInStack++;
        
        bool res = false; 
        for(auto node = head.load(std::memory_order_consume); node != nullptr; node = node -> next) {
            if(node -> value == value) {
                res = true;
                break;
            } 
        }

        epoch.enter();
        epoch.exit();

        threadInStack--;
        return res;
    }

    auto pop(ThreadEpoch<T>& epoch) -> std::shared_ptr<T> {
        //try to come in 
        iterMutex.lock(); //only for lock in LFStack::for_each(...);
        iterMutex.unlock();
        threadInStack++;
 
        Node<T>* oldHead = head.load();
        while(oldHead && !head.compare_exchange_weak(oldHead,oldHead -> next)) {
            usleep(SLEEP_TIME);
            //oldHead = head.load();
        }

        if(oldHead == nullptr) {
            return nullptr;
        }
        
        std::shared_ptr<T> res = std::make_shared<T>(oldHead -> value); 
        oldHead -> next == nullptr;        

        epoch.add(oldHead);
        epoch.enter();
        epoch.exit();

        threadInStack--;
        return res;
    }

    auto for_each(std::function<void(T)> fun_) -> void {
        iterMutex.lock();
        unsigned zero = 0u;
        while(!threadInStack.compare_exchange_weak(zero, 0u)) {
            usleep(SLEEP_TIME);
        } 

        for(auto node = head.load(); node != nullptr; node = node -> next) {
            fun_(node -> value);
        }

        iterMutex.unlock();
    }

    ~LFStack() {
        for(Node<T>* start = head.load(); start != nullptr;) {
            auto p = start;
            start = start -> next;
            delete p;
        }
    }
};
