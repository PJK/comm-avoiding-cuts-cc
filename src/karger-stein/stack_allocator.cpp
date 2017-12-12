//
//  stack_allocator.cpp
//  
//
//  Created by Lukas Gianinazzi on 12.07.15.
//
//

#include "stack_allocator.h"

#include <assert.h>


////
//STACK ALLOCATOR -- IMPLEMENTATION
////


stack_allocator::stack_allocator(long size) {
    //create first stack node
    stack_allocator_node * node = new stack_allocator_node(size);
    
    current_node = 0;
    nodes.push_back(node);
}

stack_allocator::~stack_allocator() {
    
    //destroy all stack nodes
    for (std::vector<stack_allocator_node*>::iterator node = nodes.begin(); node != nodes.end(); ++node) {
        delete *node;
        *node = NULL;
    }
    current_node = 0;
}

stack_state stack_allocator::enter() {
    
    stack_state state(current_node, nodes[current_node]->enter());
    
    return state;
}

void stack_allocator::leave(stack_state state) {
    
    current_node = state.current_node;
    
    for (unsigned int i=current_node+1; i<nodes.size(); ++i) {
        nodes[i]->head = 0;
    }
    
    nodes[current_node]->leave(state.state);
    
}

void stack_allocator::reserve(long size) {
    stack_state save = enter();
    allocate<char>(size);
    leave(save);
}


////
//STACK ALLOCATOR NODE -- IMPLEMENTATION
////


stack_allocator_node::stack_allocator_node(long size) {
    stack = (char*)malloc(size);
    if (stack == NULL) {
        capacity = 0;
    } else {
        capacity = size;
    }
}

stack_allocator_node::~stack_allocator_node() {
    if (stack) {
        free(stack);
        stack = NULL;
        head = 0;
        capacity = 0;
    }
}

stack_state_node stack_allocator_node::enter() {
    
    stack_state_node state;
    state.head = head;
    
    //std::cout << "c enter " << state.head << std::endl;
    
    return state;
}

void stack_allocator_node::leave(stack_state_node state) {
    
    assert (state.head <= head);
    
    //std::cout << "c leave " << state.head << std::endl;
    head = state.head;
    
    assert (invariant());
}

bool stack_allocator_node::invariant() {
    
    bool head_below_capacity = head <= capacity+alignment;
    
    assert(head_below_capacity);
    
    bool nonnegative = head >= 0 && capacity >= 0;
    assert(nonnegative);
    
    return head_below_capacity && nonnegative;
}




