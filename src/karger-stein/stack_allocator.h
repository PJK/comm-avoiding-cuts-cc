//
//  stack_allocator.h
//  
//
//  Created by Lukas Gianinazzi on 12.07.15.
//
//
//  This file provides a dynamically growing stack which can be used to allocate arrays of arbitrary type.
//  This is useful to pass dynamically sized datastructures using a stack. This improves the spatial locality of those structures and prevents excessive fragmentation.
//  The arrays returned by the allocator provide check for out of bounds errors when assertions are enabled.

#ifndef ____stack_allocator__
#define ____stack_allocator__

#include <stdio.h>
#include <assert.h>
#include <new>
#include <stdexcept>
#include <sstream>
#include <typeinfo>
#include <execinfo.h>
#include <stdlib.h>
#include <vector>
#include <iostream>


class stack_allocator_node;

//Safe array class
//if assertions are enabled every access is checked for out of bounds errors
template <typename T>
class array {
    
private:
    T * storage = NULL;
    size_t length = 0;
    
    bool within_bounds(size_t i) const {
        
        bool within = i>= 0 && i<length;
        
        if (!within) {
            
            std::ostringstream error_string;
            
            error_string << std::endl << "! trying to access out of bounds index " << i << " length is actually " << length;
            
            error_string << std::endl << "! elements of type " << typeid(T).name() << std::endl;
            
            error_string << "! stack trace: " << std::endl;
            
            void* callstack[128];
            int i, frames = backtrace(callstack, 128);
            char** strs = backtrace_symbols(callstack, frames);
            for (i = 0; i < frames; ++i) {
                error_string << "! " << strs[i] << std::endl;
            }
            free(strs);
            
            throw std::out_of_range(error_string.str());
        }
        
        return within;
    }
    
public:
    
    array() {
    }
    
    array(T * storage, size_t length) {
        assert (storage != NULL);
        
        this->storage = storage;
        this->length = length;
    }

    size_t get_length() const {
        return length;
    }
    
    T& get(long i) {
        assert (within_bounds(i));
        return storage[i];
    }
    
    T& operator[] (long i) {
        return get(i);
    }
    
    T& operator[] (long i) const {
        assert (within_bounds(i));
        return storage[i];
    }
    
    T * address(size_t i) const {//deprecated
        assert (i >= 0);
        assert (i <= length);
        return &storage[i];
    }
    
    T * begin() const {
        return &storage[0];
    }
    
    T * end() const {
        return &storage[get_length()];
    }
    
    void set(long i, T value) {
        assert (within_bounds(i));
        storage[i] = value;
    }
    
    array<T> suffix(long drop_elements) {
        assert (size_t(drop_elements) <= length);
        assert (drop_elements >= 0);
        
        array<T> suffix_array;
        suffix_array.storage = storage+drop_elements;
        suffix_array.length = length-drop_elements;
        
        return suffix_array;
    }
    
    array<T> prefix(size_t keep_elements) {
        assert (keep_elements <= length);
        assert (keep_elements >= 0);
        
        array<T> prefix_array;
        prefix_array.storage = storage;
        prefix_array.length = keep_elements;
        
        return prefix_array;
    }
    
};

////
//STACK STATES -- OPAQUE TYPES
////

class stack_state_node {
private:
    friend class stack_allocator_node;
    long head = 0;
};

class stack_state {
private:
    friend class stack_allocator;
    
    stack_state(int current_node, stack_state_node node_state) {
        this->state = node_state;
        this->current_node = current_node;
    }
    
    stack_state_node state;
    int current_node = 0;
};

////
//STACK ALLOCATOR
///

class stack_allocator {
    
    std::vector<stack_allocator_node*> nodes;
    unsigned int current_node = 0;
    
    template <typename T>
    void get_new_node(long size);
    
public:
    
    stack_allocator(long size);
    ~stack_allocator();
    
    template <typename T>
    array<T> allocate(long size);//allocate array with given number of elements
    
    stack_state enter();//open a new stack frame
    
    void leave(stack_state state);//restore the stack to the saved state
    
    void reserve(long size);//prealloactes the given number of bytes. This makes sense if an asymptotic estimate of the required size is known
    
};


////
//STACK ALLOCATOR NODE
////

class stack_allocator_node {
    
    static const long alignment = 8;
    
    friend class stack_allocator;
    
    char * stack = NULL;
    long capacity = 0;//the length of the stack in char
    long head = 0;//the first free element is at stack+head
    bool invariant();
    
    long remaining() {//if remaining() > 0, then remaining() bytes are available. Otherwise no bytes are available.
        return capacity-head;
    }
    
public:
    
    stack_allocator_node(long size);
    ~stack_allocator_node();
    
    template <typename T>
    array<T> allocate(long size);
    
    stack_state_node enter();
    
    void leave(stack_state_node state);
    
    
    static inline long next_address(long current) {
        //returns the next highest address that is a multiple of the alignment
            
        return (current + alignment - 1) & ~(alignment - 1);
        
        //the above is equivalent to the following code
        //if (current % alignment == 0) return current;
        //return current + alignment - (current % alignment);
    }
    
};

////
//STACK ALLOCATOR -- IMPLEMENTATION OF TEMPLATE FUNCTIONS
////

template <typename T>
array<T> stack_allocator::allocate(long size) {
    
    //std::cout << "c allocate " << size << " elements of type " << typeid(T).name() << std::endl;
    if (nodes[current_node]->remaining() < size*(long)sizeof(T)) {
        get_new_node<T>(size);
    }
    
    return nodes[current_node]->allocate<T>(size);
}

template <typename T>
void stack_allocator::get_new_node(long size) {
    
    //std::cout << "c allocate " << size << " elements of type " << typeid(T).name() << std::endl;
    while (nodes[current_node]->remaining() < size*(long)sizeof(T)) {
        
        if (nodes.size() <= current_node+1) {
            //std::cout << "c allocate new stack node " << std::endl;
            
            stack_allocator_node * new_node = new stack_allocator_node(2 * std::max<long>(size*sizeof(T), nodes[current_node]->capacity));
            if (new_node == NULL || new_node->stack == NULL) throw std::runtime_error("! stack_allocator: out of memory.\n");
            
            nodes.push_back(new_node);
        }
        ++current_node;
    }
    
}

////
//STACK ALLOCATOR NODE -- IMPLEMENTATION OF TEMPLATE FUNCTIONS
////

template <typename T>
array<T> stack_allocator_node::allocate(long size) {
    
    //std::cout << "c allocate at head " << head << std::endl;
    
    assert (head + size*(long)sizeof(T) <= capacity);
    /*if (head + size*(long)sizeof(T) > capacity) {
        
        std::ostringstream error_string;
        
        error_string << "! stack_allocator: stack overflow\n" << "the capcacity of the stack is " << capacity << ", the current head is at " << head << " and the size requested is " << size << " elements of type " << typeid(T).name() << std::endl;
        
        throw std::out_of_range(error_string.str());
    }*/
    
    array<T> safe_array((T*)(stack+head), size);
    head += size*sizeof(T);
    head = next_address(head);
    
    assert (invariant());
    
    return safe_array;
}


#endif /* defined(____stack_allocator__) */
