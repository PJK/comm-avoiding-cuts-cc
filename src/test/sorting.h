//
//  sorting.h
//  
//
//  Created by Lukas Gianinazzi on 11.10.13.
//
//

#ifndef _sorting_h
#define _sorting_h

//if key1 >  key2 : return value > 0
//if key1 <  key2 : return value < 0
//if key1 == key2 : return value == 0
typedef int (*comparator_t)(const void * key1, const void * key2);



#endif
