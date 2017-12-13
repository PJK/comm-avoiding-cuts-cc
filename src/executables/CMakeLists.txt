file(GLOB KS_FILES ../karger-stein/*.cpp)
file(GLOB RC_FILES ../recursive-contract/*.cpp)
file(GLOB SOURCES ../*.cpp)


add_executable(square_root square_root.cpp ${SOURCES} ../input/GraphInputIterator.cpp ${KS_FILES} ${RC_FILES})
target_link_libraries(square_root ${MPI_LIBRARIES})

add_executable(seq_square_root seq_square_root.cpp ${SOURCES} ../input/GraphInputIterator.cpp ${KS_FILES} ${RC_FILES})
target_link_libraries(seq_square_root ${MPI_LIBRARIES})

add_executable(transition transition.cpp ${SOURCES} ../input/GraphInputIterator.cpp ${KS_FILES} ${RC_FILES})
target_link_libraries(transition ${MPI_LIBRARIES})

add_executable(boost_stoer_wagner boost_stoer_wagner.cpp ../input/GraphInputIterator.cpp)
add_executable(karger_stein karger_stein.cpp ../input/GraphInputIterator.cpp ${KS_FILES})

add_executable(sorting_test sorting_test.cpp ../utils.cpp ../MPICollector.cpp)
target_link_libraries(sorting_test ${MPI_LIBRARIES})

add_executable(parallel_cc parallel_cc.cpp ../MPIDatatype.cpp ../MPICollector.cpp ../utils.cpp ../UnweightedIteratedSparseSampling.cpp ../MPICollector.cpp ../input/GraphInputIterator.cpp ${KS_FILES} ${RC_FILES})
target_link_libraries(parallel_cc ${MPI_LIBRARIES})

add_executable(click_parallel_cc click_parallel_cc.cpp ${SOURCES} ../input/GraphInputIterator.cpp ${KS_FILES} ${RC_FILES})
target_link_libraries(click_parallel_cc ${MPI_LIBRARIES})

add_executable(pbgl_cc pbgl_cc.cpp ../input/GraphInputIterator.cpp ../utils.cpp)
target_link_libraries(pbgl_cc ${MPI_LIBRARIES} ${Boost_LIBRARIES})

add_executable(bgl_cc bgl_cc.cpp ../input/GraphInputIterator.cpp ../utils.cpp)
target_link_libraries(bgl_cc ${Boost_LIBRARIES})

add_executable(approx_cut approx_cut.cpp ${SOURCES} ../input/GraphInputIterator.cpp ${KS_FILES} ${RC_FILES})
target_link_libraries(approx_cut ${MPI_LIBRARIES})

add_executable(connectivity connectivity.cpp ../input/GraphInputIterator.cpp)
add_executable(simple_generator simple_generator.cpp)
add_executable(levine_transform levine_transform.cpp)
add_executable(complete_graph_generator complete_graph_generator.cpp)
add_executable(normalize_gplus normalize_gplus.cpp)

if (WITH_METIS)
    add_executable(metis_cut metis.cpp ../input/GraphInputIterator.cpp ../utils.cpp)
    target_link_libraries(metis_cut ${MPI_LIBRARIES})
    target_link_libraries(metis_cut parmetis)
endif (WITH_METIS)

if (WITH_GALOIS)
    add_executable(galois_cc galois_cc.cpp)
    target_link_libraries(galois_cc ${GALOIS_LIB})

    add_executable(graph2galois graph2galois.cpp ../input/GraphInputIterator.cpp ../utils.cpp)
    target_link_libraries(graph2galois ${GALOIS_LIB})
endif (WITH_GALOIS)

add_executable(er_generator er_generator.cpp)
add_executable(sw_generator sw_generator.cpp)
