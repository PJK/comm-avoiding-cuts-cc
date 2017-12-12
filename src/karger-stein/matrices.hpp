//
//  matrices.hpp
//  
//
//  Created by Lukas Gianinazzi on 22.03.15.
//
//

#ifndef _matrices_hpp
#define _matrices_hpp


///////
///TRANSPOSE
//////


class matrices {
    template <class T>
    static void matrixOperations_transpose_naive(T * source, T * transposed, int m, int n, int source_rowstride, int transposed_rowstride);
    

public:
    
    template <class T>
    static void matrixOperations_transpose_co(T * source, T * transposed, int m, int n, int source_rowstride, int transposed_rowstride);
    
    template <class T>
    static void transpose(T * source, T * transposed, int m, int n);
};


template <class T>
void matrices::matrixOperations_transpose_naive(T * source, T * transposed, int m, int n, int source_rowstride, int transposed_rowstride)
{
    for (int i=0; i<m; i++) {
        for (int j=0; j<n; j++) {
            transposed[j*transposed_rowstride+i] = source[i*source_rowstride+j];
        }
    }
}

//cache oblivious matrix transpose
//transposes the mxn row major matrix source and stores it in row major at transposed
template <class T>
void matrices::matrixOperations_transpose_co(T * source, T * transposed, int m, int n, int source_rowstride, int transposed_rowstride)
{
    assert (source_rowstride >= 1 && transposed_rowstride >= 1 && m>=1 && n>= 1);
    
    if ( (m <= 32) & (n <= 32)) {
        matrixOperations_transpose_naive<T>(source, transposed, m, n, source_rowstride, transposed_rowstride);
        return;
    }
    if (m > n) {
        int mh = m/2;
        //#pragma omp task if (m > 2*PARALLEL_BASE_CASE_SIZE)
        matrixOperations_transpose_co(source, transposed, mh, n, source_rowstride, transposed_rowstride);
        //#pragma omp task if (m > 2*PARALLEL_BASE_CASE_SIZE)
        matrixOperations_transpose_co(&source[mh*source_rowstride], &transposed[mh], m-mh, n, source_rowstride, transposed_rowstride);
    } else {
        int nh = n/2;
        //#pragma omp task if (n > 2*PARALLEL_BASE_CASE_SIZE)
        matrixOperations_transpose_co(source, transposed, m, nh, source_rowstride, transposed_rowstride);
        //#pragma omp task if (n > 2*PARALLEL_BASE_CASE_SIZE)
        matrixOperations_transpose_co(&source[nh], &transposed[nh*transposed_rowstride], m, n-nh, source_rowstride, transposed_rowstride);
    }
}

//transposes the mxn row major matrix 'source' and stores the result in the nxm row major matrix 'transposed'
template <class T>
void matrices::transpose(T * source, T * transposed, int m, int n)
{
    //baseline algo
    //_matrixOperations_transpose_naive(source, transposed, m, n, n, m);
    
    //parallelization does speed up the computation, but is more work intensive (relatively high overhead)
    //#pragma omp parallel
    {
        //#pragma omp single
        //matrixOperations_transpose_naive(source, transposed, m, n, n, m);
        matrixOperations_transpose_co(source, transposed, m, n, n, m);
        //#pragma omp taskwait
    }
}


#endif
