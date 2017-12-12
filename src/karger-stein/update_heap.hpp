//
//  update_heap.hpp
//  
//
//  Created by Lukas Gianinazzi on 23/06/15.
//
//

#ifndef _update_heap_hpp
#define _update_heap_hpp

#include "stack_allocator.h"


////
//UPDATE HEAP -- HEADER
//
//Priority queue which supports batched update operations
//Updating O(k) keys in a queue containing n keys
//costs O(min(n, k log n)) time and O(min(n/B+1, log(n/B+1))) cache misses, where B is the width of the cache lines
//the number of elements in the queue is fixed
////

template <class T>
class update_heap {
    
    array<T> heap;
    int size;
    
    //zeros out all of the 32 lower bits except the highest bit
    //if x uses only the first 32 bits, this returns the hyperfloor(x) - i.e. the largest power of two that is not larger than x
    static int msb32(register int x)
    {
        x |= (x >> 1);
        x |= (x >> 2);
        x |= (x >> 4);
        x |= (x >> 8);
        x |= (x >> 16);
        return (x & ~(x >> 1));
    }
    
    bool invariant() const {
        return heap_property(0);
    }
    
    static int left_child(int i) {
        return 2*i+1;
    }
    
    static int right_child(int i) {
        return 2*i+2;
    }
    
    bool heap_property (int i) const;
    
    size_t heap_length() const {
        return 2*size-1;
    }
    
    int number_of_updates = 0;
    
    array<int> updates;
    
public:
    
    update_heap(int number_of_elements, T initial_value, stack_allocator * storage);
    
    T peek_max() const;
    
    int peek_max_index() const;
    
    void update();

    static size_t size_for_number_of_elements(size_t length) {//returns next higher power of two (or the length itself if length is a power of two)
        return (size_t)msb32(length) == length ? length : 2*msb32(length);
    }
    
    void increment_value(int index, T value);
    void set_value(int index, T value);
};


////
//UPDATE HEAP -- IMPLEMENTATION (Constructors)
////

template <class T>
update_heap<T>::update_heap(int number_of_elements, T initial_value, stack_allocator * storage) {
    
    size = size_for_number_of_elements(number_of_elements);
    assert (size >= number_of_elements);

    heap = storage->allocate<T>(heap_length());
    
    updates = storage->allocate<int>(number_of_elements);
    
    assert (heap.begin());
    assert (heap.end());
    
    std::fill(heap.begin(), heap.end(), initial_value);
    
    assert (invariant());
}


////
//UPDATE HEAP -- IMPLEMENTATION (Public Methods)
//
//The implementation differs slightly from the description in [Gianinazzi 2015] in that the nodes of the tree only contain the values of the maximum, not the value and the index of the leaf. This saves half of the space. A leaf that has the maximum value can easily be found by navigating from the root downwards, as in a classical heap.
////

template <class T>
T update_heap<T>::peek_max() const {
    return heap[0];
}

template <class T>
int update_heap<T>::peek_max_index() const {
    
    int i = 0;
    
    while (i < size-1) {
        
        i = left_child(i);
        
        if (heap[i+1] > heap[i]) {
            i += 1;
        }
        
        assert (i >= 0);
        assert ((size_t)i < heap_length());
        assert (heap[i] == heap[0]);
    }
    assert (i >= 0 && i >= size-1 && (size_t)i < heap_length());

    return i-(size-1);
}

template <class T>
void update_heap<T>::increment_value(int index, T value) {
    updates[number_of_updates++] = ((index+size-1)-1)/2;
    heap[index+size-1] += value;
}

template <class T>
void update_heap<T>::set_value(int index, T value) {
    updates[number_of_updates++] = ((index+size-1)-1)/2;
    heap[index+size-1] = value;
}

template <class T>
void update_heap<T>::update() {
    
    assert (number_of_updates <= size);
    //assert (std::is_sorted(updates.begin(), updates.address(number_of_updates)));

    int n = number_of_updates;
    bool running = n > 0;
    //update inner nodes
    while (running) {
        
        running = updates[0] > 0;
        
        for (int i=0; i<n; ++i) {
            
            assert (updates[i] < size);
            
            heap[updates[i]] = std::max(heap[left_child(updates[i])], heap[right_child(updates[i])]);
            
            assert (updates[i] >= 0);
            updates.set(i, (updates[i]-1)/2);
        }
        
        //int * last_index = std::unique(updates.begin(), updates.address(n));
        //n = last_index-updates.begin();
        
    }//stop after udating the root

    number_of_updates = 0;
    
    assert (invariant());
    
}

//Invariant

template <class T>
bool update_heap<T>::heap_property (int i) const {
    if (i >= size-1) return true;
    
    bool heap_property_i = heap[i] == std::max(heap[left_child(i)], heap[right_child(i)]);
    
    if (!heap_property_i) {
        std::cout << "heap property violated at index " << i << " ";
        std::cout << std::endl;
    }
    
    assert (heap_property_i);
    
    return heap_property_i && heap_property(left_child(i)) && heap_property(right_child(i));
}


#endif
