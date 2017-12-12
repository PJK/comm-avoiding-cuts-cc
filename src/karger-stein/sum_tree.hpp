//
//  sum_tree.hpp
//  
//
//  Created by Lukas Gianinazzi on 06.04.15.
//
//

#ifndef _sum_tree_hpp
#define _sum_tree_hpp

#include <algorithm>
#include <functional>
#include "test/testing_utility.h"
#include "stack_allocator.h"


template <class T>
class sum_tree {
    
    array<T> leafs;
    array<T> sums;
    size_t length;
    size_t last_layer_size;
    
    bool auxiliary_storage_is_owned_by_this_instance = false;
    
    //zeros out all of the 32 lower bits except the highest bit
    //if x uses only the first 32 bits, this returns the hyperfloor(x)
    static int msb32(register int x)
    {
        x |= (x >> 1);
        x |= (x >> 2);
        x |= (x >> 4);
        x |= (x >> 8);
        x |= (x >> 16);
        return (x & ~(x >> 1));
    }
    
    static int size_of_last_layer_for_number_of_leaves(int length) {
        return msb32(length) == length ? length / 2 : msb32(length);
    }

    bool invariant();
    
    bool monotonicity_invariant(size_t node_index, size_t layer_size);
    
public:
    
    array<T> get_tree() {
        return sums;
    }

    size_t get_length() const {
        return length;
    }
    
    static size_t number_of_elements_required(size_t length) {
        return std::max(1, (2*size_of_last_layer_for_number_of_leaves(length)));
    }
    
    static size_t space_requirement(size_t length) {
        return number_of_elements_required(length) * sizeof(T);
    }
    
    T root() {
        return sums[0];
    }
    
    ~sum_tree() {
        if (auxiliary_storage_is_owned_by_this_instance) {
            delete [] sums.begin();
        }
    }
    
    sum_tree() {
        length = 0;
        last_layer_size = 0;
    }

    // TODO this could operate on an iterator range instead
    sum_tree(const T * elements, int n) {
        auxiliary_storage_is_owned_by_this_instance = true;
        T * aux = new T[number_of_elements_required(n)];
        init(elements, aux, n);
    }
    
    void init(const T * elements, T * auxiliary_storage, int n) {
        // TODO: the constness cast is fugly and could be avoided
        array<T> elems(const_cast<T *>(elements), n);
        array<T> aux(auxiliary_storage, number_of_elements_required(n));
        init(elems, aux);
        //test_printArray(sums, length);
    }
    
    sum_tree(array<T> elements, array<T> auxiliary_storage) {
        
        init(elements, auxiliary_storage);
        //test_printArray(sums, length);
    }
    
    void init(array<T> elements, array<T> auxiliary_storage) {
        size_t length = elements.get_length();
        
        
        assert (auxiliary_storage.get_length() >= number_of_elements_required(length));
        
        leafs = elements;
        sums = auxiliary_storage;
        
        this->length = length;
        
        if (length <= 1) {
            assert (auxiliary_storage.get_length() >= 1);
            if (length > 0) {
                sums[0] = auxiliary_storage[0];
            }
            return;
        }
        
        assert(length > 1);
        last_layer_size = size_of_last_layer_for_number_of_leaves(length);
        
        T * src_layer;
        
        //std::fill(sums, sums+number_of_elements_required(length), 0);
        
        //int canary = sums[space_requirement(length)];
        //int canary = sums[length-1];
        
        {
            size_t remaining = length-last_layer_size;
            
            //printf("length %d, last layer %d, remaining %d \n", length, last_layer_size, remaining);
            
            assert (remaining <= last_layer_size);
            assert (remaining >= 0);
            assert (2*remaining+last_layer_size-remaining == length);
            
            T * cur_layer = sums.address(last_layer_size-1);
            
            assert (cur_layer + remaining <= sums.address(length));
            
            std::transform(leafs.begin(), leafs.address(remaining), leafs.address(last_layer_size), cur_layer, std::plus<T>());
            std::copy(leafs.address(remaining), leafs.address(last_layer_size), cur_layer + remaining);
            src_layer = cur_layer;
        }
        
        for (size_t src_layer_size { last_layer_size }; src_layer_size > 1; src_layer_size = src_layer_size/2) {
            //printf("src_layer_size %d \n", src_layer_size);
            
            T * dst_layer = src_layer - src_layer_size/2;
            
            assert(dst_layer >= sums.begin());
            
            std::transform(src_layer, src_layer+src_layer_size/2, src_layer+src_layer_size/2, dst_layer, std::plus<T>());
            
            src_layer = dst_layer;
            
        }
        
        //test_printArray(sums, 2*last_layer_size-1);
        
        assert (this->length == length);
        
        assert (invariant());
        //assert (sums[space_requirement(length)] == canary);
    }
    
    //find the index of the first leaf that is greater or equal to the given value
    size_t lower_bound(T value) {
        
        if (length == 1) {
            return 0;
        }
        
        assert (value >= 0);
        assert (value <= root());

        size_t index = 0;
        size_t cur_layer = 1;
        
        while (cur_layer < last_layer_size) {
            //printf("value %d, layer size %d, index %d, sums[index] %d\n", value, cur_layer, index, sums[index]);
            
            assert (value <= sums[index]);

            size_t child_index = index+cur_layer;
            
            if (sums[child_index] < value) {
                value -= sums[child_index];
                child_index += cur_layer;
            }
            
            index = child_index;
            cur_layer = 2*cur_layer;
        }
        //printf("index %d \n", index);
        
        index = index-(last_layer_size-1);//relative to beginning of last layer
        
        
        //printf("rel index %d \n", index);
        
        assert (index >= 0);

        if (index < (length-last_layer_size) && leafs[index] < value) {
            index += last_layer_size;
        }
        
        //printf("choose index %d \n", index);
        assert (leafs[index] > 0);
        assert (index < length);
        
        return index;
    }
    
    
    void update(int index, T new_value) {
        
        if (length == 1) {
            assert (index == 0);
            sums[0] = new_value;
            return;
        }
        
        bool decrease = new_value < leafs[index];
        T difference;
        
        //some types cannot represent negative numbers
        //thus, we compute the absolute difference, and either add or subtract it
        if (decrease) {
            difference = leafs[index]-new_value;
        } else {
            difference = new_value-leafs[index];
        }
        
        //printf("update index %d to %llu - old value %llu - difference %llu\n", index, new_value, leafs[index], difference);
        
        leafs[index] = new_value;
        
        if ((size_t) index >= last_layer_size) {
            index -= last_layer_size;
        }
        
        index += last_layer_size-1;//relative to beginning of sums

        size_t cur_layer = last_layer_size;
        do {
            //printf("update index %d, cur_layer %d \n", index, cur_layer);
            if (decrease) {
                assert (sums[index] - difference <= sums[index]);
                assert (sums[index] >= difference);
                sums.set(index, sums[index] - difference);
            } else {
                assert (sums[index] + difference >= sums[index]);
                sums.set(index, sums[index] + difference);
            }
            assert(sums[index] >= 0);
            
            index -= cur_layer/2;
            if ((size_t) index >= (cur_layer-1)) {
                index -= cur_layer/2;
            }
            cur_layer = cur_layer/2;
        } while (cur_layer > 0);
        
        assert(invariant());
        //test_printArray(sums, space_requirement(length));
    }
    
};


template<class T>
bool sum_tree<T>::invariant() {
    
    if (length == 1) {
        return true;
    }
    
    bool inv = monotonicity_invariant(0, 1);
    //bool inv = true;
    for (T * node = sums.begin(); node < sums.address(number_of_elements_required(length)-1); ++node) {
        bool nonnegative = *node >= 0;
        assert (nonnegative);
        inv = inv && nonnegative;
    }
    size_t beginning_of_last_layer = last_layer_size-1;
    size_t length_of_last_layer_sum = length-last_layer_size;
    
    for (size_t i { 0 }; i<length_of_last_layer_sum; ++i) {
        bool is_sum_of_two_leaves = sums[beginning_of_last_layer+i] == leafs[i]+leafs[i+last_layer_size];
        assert (is_sum_of_two_leaves);
        inv = inv && is_sum_of_two_leaves;
    }
    for (size_t i { length_of_last_layer_sum }; i<last_layer_size; ++i) {
        bool is_same_as_leaf = sums[beginning_of_last_layer+i] == leafs[i];
        assert (is_same_as_leaf);
        inv = inv && is_same_as_leaf;
    }
    
    return inv;
}

template<class T>
bool sum_tree<T>::monotonicity_invariant(size_t node_index, size_t layer_size) {
    
    if (layer_size >= last_layer_size) {
        return true;
    }
    
    int left_child = node_index +layer_size;
    int right_child = node_index + 2*layer_size;
    
    bool monotone = sums[node_index] >= sums[left_child] && sums[node_index] >= sums[right_child];
    bool sum_requirement = sums[node_index] == sums[left_child] + sums[right_child];
    
    if (!monotone || !sum_requirement) {
        std::cout << "invariant violated: root " << sums[node_index] << ", children " <<  sums[left_child] << ", " <<  sums[right_child] << " -- acutal sum " << sums[left_child]+sums[right_child] << std::endl << "sizeof(T)" << (sizeof(T)*1) << std::endl;
    }
    
    assert (sum_requirement);
    assert (monotone);
    
    return monotone && sum_requirement && monotonicity_invariant(right_child, layer_size*2) && monotonicity_invariant(left_child, layer_size*2);
}

#endif
