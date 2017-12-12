//
//  parallel_contract.cpp
//  
//
//  Created by Lukas Gianinazzi on 13.10.15.
//
//

#include "parallel_contract.hpp"
#include "connected_components.hpp"
#include "matrices.hpp"
#include "MPICollector.hpp"

namespace mincut {
    
    int number_of_edges_to_sample_r(int number_of_vertices) {
        return int(pow(number_of_vertices, 1.2)) + 1;
    }
    
    //Selects for every processor a number of edges to select
    //Every edge is assigned to some processor P with probability proportional to the sum of all edges stored in processor P
    //The routine selects the order in which edges were assigned to the processors
    void select_number_of_edges_per_processor(int p, int number_of_edges_to_select, long * sums, int ** edges_per_processor, int ** edges_processor_order, sitmo::prng_engine * random_engine) {
        
        assert (p>1);
        assert (number_of_edges_to_select>0);
        assert (edges_processor_order);
        assert (edges_per_processor);
        assert (random_engine);
        assert (sums);
        
        //std::cout << "choose " << number_of_edges_to_select << std::endl;
        
        *edges_per_processor = new int[p];
        *edges_processor_order = new int[number_of_edges_to_select];

        sum_tree<long> index(sums, p);//The index has a representation similar to a prefix-sum, it allows to quickly select indices with probability proporional to their weight
        
        std::fill(*edges_per_processor, *edges_per_processor+p, (int)0);
        
        long sum = index.root();
        
        std::uniform_int_distribution<long> uniform_int(1, sum);

        for (int i=0; i<number_of_edges_to_select; ++i) {
            
            long r = uniform_int(*random_engine);
            int selection = index.lower_bound(r);
            
            (*edges_per_processor)[selection] += 1;
            (*edges_processor_order)[i] = selection;
            
            //std::cout << "processor " << selection << std::endl;
        }
        
    }
    
    void select_edges(int number_of_edges_to_select, edge_struct_t * select_edges, graph_slice_index<long> * prefix_sums, sitmo::prng_engine * random_engine) {
        
        for (int i=0; i<number_of_edges_to_select; ++i) {
            prefix_sums->select_random_edge(&select_edges[i], random_engine);
        }
        
    }
    
    int rank_labels(int * source, int * destination, int n) {
        //mark all values that appear with a 1
        std::fill<int*>(destination, destination+n, 0);
        for (int i=0; i<n; ++i) {
            destination[source[i]] = 1;
        }
        //the partial sums almost give the rank (1 based so we need to subtract 1 to get 0-based ranks)
        destination[0] = destination[0]-1;
        std::partial_sum<int*>(destination, destination+n, destination);
        
        int v = destination[n-1]+1;
        
        //extend the rank to those which do not have themselves as label
        for (int i = 0; i<n; ++i) {
            destination[i] = destination[source[i]];
        }
        
        return v;
    }


    /** FIXME: PROFILING? You'll know better **/
    void parallel_sample_edges(MPI_Comm comm, graph_slice<long> * graph, sitmo::prng_engine * random_generator, edge_struct_t * edge_sample) {
        
        int p;
        int rank;
        MPI_Comm_size(comm, &p);
        MPI_Comm_rank(comm, &rank);
        
        long sum = 0;
        long * sums = NULL;
        
        assert (graph != NULL);
        
        if (rank == 0) {
            //Master process
            sums = new long[p];
            
        }
        
        sum = graph->accumulate();
        
        
        MPI::Gather(&sum, 1, MPI_LONG, sums, 1, MPI_LONG, 0, comm);
        
        int * edges_per_processor = NULL;
        int number_of_edges_to_select = 0;
        int * edges_processor_order = NULL;
        graph_slice_index<long> * prefix_sums = NULL;
        int sample_size = number_of_edges_to_sample_r(graph->get_number_of_vertices());
        
        
        if (rank == 0) {
            //select how many edges per processor
            select_number_of_edges_per_processor(p, sample_size, sums, &edges_per_processor, &edges_processor_order, random_generator);
            
            assert (edges_per_processor);
            assert (edges_processor_order);
            
        }
        // compute the prefix sums
        prefix_sums = new graph_slice_index<long>(graph);
        
        
        MPI::Scatter(edges_per_processor, 1, MPI_INT, &number_of_edges_to_select, 1, MPI_INT, 0, comm);


        edge_struct_t * selected_edges = NULL;
        int * recv_displacements = NULL;
        
        
        if (rank == 0) {
            selected_edges = new edge_struct_t[sample_size];
            
            //The edges of the other processors are received next to each other => use prefix scan to compute the displacements
            recv_displacements = new int[p];
            recv_displacements[0] = 0;
            std::partial_sum(edges_per_processor, edges_per_processor+p-1, recv_displacements+1);
            
        } else {
            selected_edges = new edge_struct_t[number_of_edges_to_select];
        }
        //select as many random edges as indicated by the root

        /** FIXME: PROFILING? You'll know better **/
        select_edges(number_of_edges_to_select, selected_edges, prefix_sums, random_generator);
        
        
        
        //Gather the selected edges at the root, in processor order
        MPI::Gatherv(rank ? selected_edges : MPI_IN_PLACE, number_of_edges_to_select, MPI_2INT, selected_edges, edges_per_processor, recv_displacements, MPI_2INT, 0, comm);
        
        if (rank == 0) {
            //root permutes edges according to the order edges were assigned to processors to begin with
            //If we would leave this step out, the edges would not have the right distribution: edges with smaller endpoints would be more likely to come early in the permutation
            
            for (int i=0; i<sample_size; ++i) {
                assert (recv_displacements[edges_processor_order[i]] < sample_size);
                
                edge_sample[i] = selected_edges[recv_displacements[edges_processor_order[i]]++];
            }

        }
        
        if (recv_displacements) delete[] recv_displacements;
        if (edges_processor_order) delete[] edges_processor_order;
        if (sums) delete[] sums;
        if (prefix_sums) delete prefix_sums;
        if (edges_per_processor) delete[] edges_per_processor;
        if (selected_edges) delete [] selected_edges;
    }
    
    void distributed_matrix_transpose(long * src, long * dest, int k, int v, MPI_Comm comm);
    
    int parallel_contract_try(MPI_Comm comm, graph_slice<long> * graph, sitmo::prng_engine * random_generator, int target_v);
    
    void parallel_contract(MPI_Comm comm, graph_slice<long> * graph, sitmo::prng_engine * random_generator, int target_v) {
        
        int cur = graph->get_number_of_vertices();
        
        while ((cur = parallel_contract_try(comm, graph, random_generator, target_v)) > target_v) {
            //std::cout << "v: " << cur << "/ target: " << target_v << std::endl;
        };
    }
    
    int parallel_contract_try(MPI_Comm comm, graph_slice<long> * graph, sitmo::prng_engine * random_generator, int target_v) {
    
        int p;
        int rank;
        
        MPI_Comm_size(comm, &p);
        MPI_Comm_rank(comm, &rank);
    
        int v = graph->get_size();
        int k = graph->get_rows_per_slice();
        
        assert (p > 1);
        assert (k*p == v);
        
        int virtual_v = graph->get_number_of_vertices();
        
        edge_struct_t * edge_sample = NULL;
        
        if (rank == 0) {
            edge_sample = new edge_struct_t[number_of_edges_to_sample_r(virtual_v)];
        }
        
        parallel_sample_edges(comm, graph, random_generator, edge_sample);
        
        int * relabeling = new int[virtual_v+1];
        
        int actual_v;
        
        if (rank == 0) {
            //root performs CC computation

            int * unnormalized_relabeling = new int[virtual_v];
            
            size_t prefix_length;
            
            //std::cout << virtual_v << std::endl;
            
            prefix_connected_components(edge_sample, (size_t) number_of_edges_to_sample_r(virtual_v), unnormalized_relabeling, virtual_v, target_v, &prefix_length);

            //std::iota(relabeling+virtual_v, relabeling+v, virtual_v);

            actual_v = rank_labels(unnormalized_relabeling, relabeling, virtual_v);

            assert (actual_v >= target_v);
            
            delete[] unnormalized_relabeling;
            delete[] edge_sample;
            edge_sample = NULL;
            
            relabeling[virtual_v] = actual_v;
        }

        //root broadcasts CC computation results
        //MPI_Bcast(&actual_v, 1, MPI_INT, 0, comm);
        MPI::Bcast(relabeling, virtual_v+1, MPI_INT, 0, comm);
        actual_v = relabeling[virtual_v];

        //Contract the graph by combining rows and columns of the matrix
        //This is done by first combining columns locally, then performing a distributed matrix transpose, and then again combining columns locally. Note that combining columns of the transposed matrix corresponds to combining the rows of the original matrix
        //Finally, the diagonal of the matrix is zeroed, so that loops are removed
        
        //graph->print(comm);
        
        long * aux_graph = new long[(long)v * k];
        graph_slice<long> shrinked_graph(virtual_v, k, graph->get_rank(), v, aux_graph);
            
        //nodes locally combine columns of the matrix
        //graph->combine_cols_using_relabeling(&shrinked_graph, relabeling);

        //shrinked_graph.print(comm);
        
        /*for (int i=0; i<virtual_v; i+=k) {//Locally transpose blocks
            matrices::matrixOperations_transpose_co(graph->get(0,i), shrinked_graph.get(0,i), k, k, v, v);
        }*/
        
        matrices::transpose(graph->begin(), shrinked_graph.begin(), k, v);
        
        //shrinked_graph.print(comm);
        
        
        shrinked_graph.combine_cols_using_relabeling_t(graph, relabeling);
        
        //graph->print(comm);
        
        //graph->print(comm);
        
        //std::copy(shrinked_graph.begin(), shrinked_graph.get_row(k), graph->begin());

        /** FIXME: PROFILING? You'll know better **/
        //Distributed Matrix Transpose
        distributed_matrix_transpose(graph->begin(), shrinked_graph.begin(), k, v, comm);
        //distributed_matrix_transpose(shrinked_graph.begin(), graph->begin(), k, v, comm);

        
        //shrinked_graph.print(comm);
        
        /*for (int i=0; i<virtual_v; i+=k) {//Locally transpose blocks
            matrices::matrixOperations_transpose_co(shrinked_graph.get(0,i), graph->get(0,i), k, k, v, v);
        }*/
        
        for (long i=0; i<virtual_v; i+=k) {//Locally transpose blocks
            matrices::matrixOperations_transpose_co(shrinked_graph.begin()+i*k, graph->begin()+i*k, k, k, k, k);
        }

        /** FIXME: PROFILING? You'll know better **/
        graph->combine_cols_using_relabeling_t(&shrinked_graph, relabeling);
        

        matrices::transpose(shrinked_graph.begin(), graph->begin(), v, k);
        
        
        //nodes locally combine columns of the matrix
        //shrinked_graph.combine_cols_using_relabeling(graph, relabeling);

        graph->remove_loops();
        
        
        //graph->print(comm);

        delete[] aux_graph;
        
        graph->set_number_of_vertices(actual_v);
        
        if (relabeling) delete[] relabeling;
        
        return actual_v;
    }

    ////
    //Distributed Matrix Transpose (Using Datatypes)
    //TODO : This should probably go into another file
    ////
    
    /*MPI_Datatype create_column_t(int k, int v) {
    
        MPI_Datatype column_major_block_t;
        
        MPI_Datatype column_t;
        MPI_Type_vector(k, 1, v, MPI_LONG, &column_t);
        
        MPI_Type_commit(&column_t);
        
        MPI_Aint sizeofT;
        MPI_Aint lb;
        MPI_Type_get_extent(MPI_LONG, &lb, &sizeofT);
        
        
        MPI_Type_create_resized(column_t, 0, sizeofT, &column_major_block_t);
        
        MPI_Datatype cm;
        
        MPI_Type_create_hvector(k, 1, sizeofT, column_t, &cm);
        
        
        MPI_Type_commit(&column_major_block_t);
        MPI_Type_commit(&cm);
        //MPI_Type_free(&column_t);
        

        return cm;
    }*/
    
    /*
    MPI_Datatype create_row_major_block_t(int k, int v) {
    
        MPI_Datatype row_major_block_t;
        
        MPI_Datatype row_t;
        MPI_Type_contiguous(k, MPI_LONG, &row_t);

        MPI_Datatype row_block_t;//This captures a row major block, but its extent is too large so that concatenating the datatype would give a matrix
        MPI_Type_vector(k, k, v, MPI_LONG, &row_block_t);

        MPI_Aint sizeofT;
        MPI_Aint lb;
        MPI_Type_get_extent(row_t, &lb, &sizeofT);
        
        MPI_Type_create_resized(row_block_t, 0, sizeofT, &row_major_block_t);
        MPI_Type_commit(&row_major_block_t);

        return row_major_block_t;
    }*/
    
    void distributed_matrix_transpose(long * src, long * dest, int k, int v, MPI_Comm comm) {
        //MPI_Datatype row_major_block_t = create_row_major_block_t(k, v);
        
        //MPI_Alltoall(src, 1, row_major_block_t, dest, 1, row_major_block_t, comm);
        
        MPI::Alltoall(src, k*k, MPI_LONG, dest, k*k, MPI_LONG, comm);
        
        
        //MPI_Type_free(&row_major_block_t);
    }
    

}
