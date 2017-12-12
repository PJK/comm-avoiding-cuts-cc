//
//  testingUtility.h
//  
//
//  Created by Lukas Gianinazzi on 29.09.13.
//
//

#ifndef _testingUtility_h
#define _testingUtility_h

#include <assert.h>
#include <iostream>
#include "sorting.h"

int test_contains(int * array, int length, int key);

int * test_getRandomSequence(int length, int upperbound);
int * test_getDecreasingSequence(int length);
int * test_getAscendingSequence(int length);

//returns if the array is sorted (nondecreasing) with respect to the provided comparator
//comparator convention:
//if key1 >  key2 : return value > 0
//if key1 <  key2 : return value < 0
//if key1 == key2 : return value == 0
int test_isSorted(void * sequence, comparator_t, size_t elementSize, int numElements);

//only actually prints the array if neither NDEBUG nor NDEBUGPRINT is defined
int test_printArray(unsigned long long * array, int length);
int test_printArray(int * array, int length);

//template <class T>
//int test_print_array<T>(T * array, int length);


//comparator_t for integers
int int_compare(const void * key1, const void *  key2);


template <class T>
int test_print_array(T * array, int length)
{
#ifndef NDEBUG
#ifndef NDEBUGPRINT
    
    std::cout << "[";
    for (int i=0; i<length; i++) {
        std::cout << array[i] << " ";
    }
    std::cout << "]" << std::endl;
#endif
#endif
    return 1;
}

#endif