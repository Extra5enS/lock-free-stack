#pragma once

#include<mutex>
#include<atomic>
#include<unistd.h>
#include<functional>
#include<memory>
#include<vector>

#define SLEEP_TIME 5
#define EPOCH_COUNT 3

template<class T>
struct Node {
    T value;
    Node* next;
    Node(T const& value): value(value) {}
    ~Node() { delete next; }
};


template<class T>
std::vector<std::vector<Node<T>* > > arrayForErase;
std::atomic<unsigned> globalEpoch;
std::atomic<unsigned> threadsInNextEpoch;
std::atomic<unsigned> threadCount;

template<class T>
struct ThreadEpoch {
    unsigned threadEpoch;

    ThreadEpoch(): threadEpoch(1) {
        arrayForErase<T>.resize(EPOCH_COUNT);
        globalEpoch.store(1);
        threadsInNextEpoch.store(0);
        threadCount++;
    }

    auto add(Node<T>* value) -> void {
        value -> next = nullptr;
        arrayForErase<T>[threadCount.load() % EPOCH_COUNT].push_back(value);
    }

    auto enter() -> void {
        if(threadEpoch <= globalEpoch.load()) {
            threadEpoch = globalEpoch.load() + 1;
            threadsInNextEpoch++;
        }
    }

    auto exit() -> void {
        auto thc = threadCount.load();
        if(threadsInNextEpoch.compare_exchange_weak(thc, 0)) {
            globalEpoch++;
            int iter = (globalEpoch.load() - 1) % EPOCH_COUNT;
            arrayForErase<T>[iter].clear();
        }
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
        epoch.enter();
        
        bool itIs = false; 
        
        for(auto node = head.load(); node != nullptr; node = node -> next) {
            if(node -> value == value) {
                itIs = true;
                break;
            } 
        }
        
        epoch.exit();
        threadInStack--;
        
        return itIs;
    }

    auto pop(ThreadEpoch<T>& epoch) -> std::shared_ptr<T> {
        //try to come in 
        iterMutex.lock(); //only for lock in LFStack::for_each(...);
        iterMutex.unlock();

        threadInStack++;
        epoch.enter();

        //TODO impliment hazard pointer
        //or Epoch-based reclamation <- we will use it!
        Node<T> *oldHead = head.load();
        while(!head.compare_exchange_weak(oldHead,oldHead -> next)) {
            usleep(SLEEP_TIME);
        }
        std::shared_ptr<T> res = std::make_shared<T>(oldHead -> value); 
        epoch.add(oldHead);
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
        delete head.load();
    }

};
