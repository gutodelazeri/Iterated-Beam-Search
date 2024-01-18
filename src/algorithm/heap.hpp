#pragma once

#include <vector>
#include <iostream>
using namespace std;

#include <boost/heap/binomial_heap.hpp>
using namespace boost::heap;

class MyHeap {
private:
    typedef struct HeapNode {
        unsigned n;
        unsigned w;
        HeapNode(unsigned _n, unsigned _w) : n(_n), w(_w) {}
    } HeapNode;
    struct CompareHeapNode {
        bool operator()(const HeapNode &n1, const HeapNode &n2) const {
            return n1.w > n2.w;
        }
    };
    binomial_heap<HeapNode, compare<CompareHeapNode>> Q;
    vector<binomial_heap<HeapNode, compare<CompareHeapNode>>::handle_type> handles;
    vector<bool> valid_handle;
public:
    MyHeap(unsigned n){
        Q = binomial_heap<HeapNode, compare<CompareHeapNode>>();
        handles = vector<binomial_heap<HeapNode, compare<CompareHeapNode>>::handle_type>(n);
        valid_handle = vector<bool>(n, false);
    }

    void clear() {
        fill(valid_handle.begin(), valid_handle.end(), false);
        Q.clear();  
    }

    inline unsigned getMinElement(){
        return Q.top().n;
    }

    unsigned findAndDeleteMinElement() {
        unsigned e = Q.top().n;
        Q.pop();
        valid_handle[e] = false;
        return e;
    }

    pair<unsigned, unsigned> getMin(){
        auto heap_element = Q.top();
        return {heap_element.n, heap_element.w};
    }

    inline void deleteMin() {
        unsigned e = Q.top().n;
        valid_handle[e] = false;
        Q.pop();
    }

    void insertElement(unsigned element, unsigned key){
        handles[element] = Q.push(HeapNode(element, key));
        valid_handle[element] = true;
    }

    void adjustHeap(unsigned element, unsigned key){
        if (valid_handle[element]) 
            Q.update(handles[element], HeapNode(element, key));
        else {
            handles[element] = Q.push(HeapNode(element, key));
            valid_handle[element] = true;
        }
    }

    inline bool empty(){
        return Q.empty();
    }

    void print_heap() {
        for (const auto &e : Q) 
            cout << "(" << e.w << ", " << e.n << ") | ";
        cout << endl;
    }
};
