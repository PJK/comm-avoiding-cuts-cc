//
//  CO-MinCut.cpp
//  
//
//  Created by Lukas Gianinazzi on 13.03.15.
//
//

#ifndef ____CO_MinCut_implementation__
#define ____CO_MinCut_implementation__


#include "lazy_adjacency_matrix.hpp"
#include "indexed_adjacency_matrix.hpp"
#include "co_mincut.h"

namespace comincut {
    int target_number_of_vertices_for_contraction(int capacity) {//never contract to less than the base case size
        return std::max((int)ceil((double)capacity/sqrt((double)recursive_fanout))+1, base_case_size);
    }

    int depth_of_recursion(int number_of_vertices) {
        int cur_v = number_of_vertices;
        int depth = 0;
        while (cur_v > base_case_size) {
            cur_v = target_number_of_vertices_for_contraction(cur_v);
            ++depth;
        }
        return depth;
    }

    double min_success_in_one_trial(int number_of_vertices) {

        if (number_of_vertices <= base_case_size) return 1;

        double recursive_success = min_success_in_one_trial(target_number_of_vertices_for_contraction(number_of_vertices));

        return 1.0 - pow((1- 0.5* recursive_success), 2);

    }

    //returns an upperbound on the number of trials needed to get the desired success probability
    int number_of_trials(int number_of_vertices, double success_probability) {

        if (number_of_vertices <= base_case_size) return 1;
        if (success_probability == 0) return 1;

        assert (recursive_fanout > 1);
        assert (success_probability > 0);
        assert (success_probability < 1);

        //estimated success probability of one iteration
        double success_in_one_trial = min_success_in_one_trial(number_of_vertices); //guaranteed to be Omega(1/log V) by the theory, but we can get better bounds
        //printf("success in one trial (general estimate) = %f \n epsilon=%f, n=%d \n", success_in_one_trial, epsilon, number_of_vertices);

        assert (success_in_one_trial > 0);
        assert (success_in_one_trial <= 1);

        //upper bound using 1+x <= e^x (is slightly worse than the computation below)
        //int number_of_trials = (int)ceil((- log(1-success_probability)) * inverse_success_in_one_trial);

        double current_failure = 1.0;
        int number_of_trials = 0;
        double target_failure = 1.0-success_probability;
        while (current_failure > target_failure) {
            current_failure = current_failure*(1.0-success_in_one_trial);
            ++number_of_trials;
        }

        //printf("# %d number of trials for success probability >= %f \n", number_of_trials, success_probability);

        return number_of_trials;
    }
}


#endif /* defined(____CO_MinCut_implementation__) */
