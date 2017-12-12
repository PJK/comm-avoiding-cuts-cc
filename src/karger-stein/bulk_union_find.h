//
//  union_find.h
//  
//
//  Created by Lukas Gianinazzi on 18.03.15.
//
//

#ifndef _bulk_union_find_h
#define _bulk_union_find_h

#include "stack_allocator.h"

class bulk_union_find {

private:
    void bulk_union_avx(int u, int v);
    void bulk_union_avx_unroll(int u, int v);
public:
    int length;
    int number_of_sets;
    array<int> rep;
    
    bulk_union_find(array<int> storage);
    
    
    void bulk_union(int u, int v);
    void compact();
    
    static int rank(int length, int * elements);
};

#endif
