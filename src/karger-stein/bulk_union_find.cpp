//
//  bulk_union_find.cpp
//  
//
//  Created by Lukas Gianinazzi on 18.03.15.
//
//

#include <stdio.h>
#include "bulk_union_find.h"
#include <numeric>
#include <assert.h>
#include <immintrin.h>
#include <x86intrin.h>
#include "test/testing_utility.h"


bulk_union_find::bulk_union_find(array<int> elements) {
    
    rep = elements;
    number_of_sets = elements.get_length();
    
    //initialize with [1..number_of_sets]
    compact();
}


int bulk_union_find::rank(int length, int * elements) {
    
    //the i'th largest representative index will have value i
    int count = 0;
    for (int i=0; i<length; ++i) {
        if (elements[i] == i) {
            elements[i] = -count-1;
            ++count;
        }
    }
    
    //extend rank to other positions
    for (int i=0; i<length; i++) {
        if (elements[i] >= 0) {
            elements[i] = -elements[elements[i]]-1;
        }
    }
    for (int i=0; i<length; i++) {
        if (elements[i] < 0) {
            elements[i] = -elements[i]-1;
        }
    }
    
    return count;
}

void bulk_union_find::bulk_union(int u, int v) {
    assert(u != v);
    assert(rep[v] == v);
    assert(number_of_sets > 0);
    number_of_sets--;
    
    //int * rep = this->rep.begin();
    
    //printf("union %d %d (length %d)\n", u, v, length);
    
    //#pragma omp parallel for
    #ifdef __AVX2__
    bulk_union_avx_unroll(u, v);
    #else
    for (int i=0; i<length; i++) {
        if (rep[i] == u) {
            rep.set(i, v);
        }
    }
    #endif
}

void bulk_union_find::bulk_union_avx(int u, int v) {
    #ifdef __AVX2__
    int * rep = this->rep.begin();

    int elements_per_vector = 8;
    int spare_iterations = length%elements_per_vector;
    
    for (int i=0; i<spare_iterations; i++) {
        if (rep[i] == u) {
            rep[i] = v;
        }
    }
    
    __m256i v_vec = _mm256_set1_epi32(v);
    __m256i u_vec = _mm256_set1_epi32(u);
    
    for (int i=spare_iterations; i<length; i+=elements_per_vector) {
        __m256i cur = _mm256_loadu_si256((__m256i*)&rep[i]);
        __m256i comparison = _mm256_cmpeq_epi32(cur, u_vec);
        
        __m256i updated = _mm256_or_si256(_mm256_and_si256(comparison, v_vec), _mm256_andnot_si256(comparison, cur));
        _mm256_storeu_si256((__m256i*)&rep[i], updated);
    }
    #else
    exit(1);
    #endif
}


void bulk_union_find::bulk_union_avx_unroll(int u, int v) {
    #ifdef __AVX2__
    int * rep = this->rep.begin();
    
    const int elements_per_vector = 8;
    const int unroll = 2;
    const int spare_iterations = length%(unroll*elements_per_vector);
    
    for (int i=0; i<spare_iterations; i++) {
        if (rep[i] == u) {
            rep[i] = v;
        }
    }
    
    __m256i v_vec = _mm256_set1_epi32(v);
    __m256i u_vec = _mm256_set1_epi32(u);
    
    for (int i=spare_iterations; i<length; i+=unroll*elements_per_vector) {
        
        __m256i cur1 = _mm256_loadu_si256((__m256i*)&rep[i]);
        __m256i cur2 = _mm256_loadu_si256((__m256i*)&rep[i+elements_per_vector]);
        
        __m256i comparison1 = _mm256_cmpeq_epi32(cur1, u_vec);
        __m256i comparison2 = _mm256_cmpeq_epi32(cur2, u_vec);
        
        __m256i updated1 = _mm256_or_si256(_mm256_and_si256(comparison1, v_vec), _mm256_andnot_si256(comparison1, cur1));
        __m256i updated2 = _mm256_or_si256(_mm256_and_si256(comparison2, v_vec), _mm256_andnot_si256(comparison2, cur2));
        
        _mm256_storeu_si256((__m256i*)&rep[i], updated1);
        _mm256_storeu_si256((__m256i*)&rep[i+elements_per_vector], updated2);
        
    }
    #else
    exit(1);
    #endif
    
}


void bulk_union_find::compact() {
    length = number_of_sets;
    rep = rep.prefix(number_of_sets);
    std::iota(rep.address(0), rep.end(), 0);
    /*for (int i=0; i<length; i++) {
        rep.set(i,i);
    }*/
}
