cmake_minimum_required(VERSION 2.8)
project(ParMETIS)

# XXX These have to be absolute
# Some genius thought it would be a good idea to make cmake using make and then run make. So there.
SET(BASEPATH "${CMAKE_SOURCE_DIR}/lib/parmetis")

set(GKLIB_PATH "${BASEPATH}/metis/GKlib" CACHE PATH "path to GKlib")
set(METIS_PATH "${BASEPATH}/metis" CACHE PATH "path to METIS")

# Symlink ./metis to wherever metis is. This allows files to be
# included from metis/libmetis/.
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${METIS_PATH} metis)

# Search for MPI.
# GK commented this out as it seems to be creating problems
# include(FindMPI)
# if(NOT MPI_FOUND)
#   message(FATAL_ERROR "mpi is not found")
# endif()
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MPI_COMPILE_FLAGS}")

# Prepare libraries.
if(SHARED)
  set(ParMETIS_LIBRARY_TYPE SHARED)
else()
  set(ParMETIS_LIBRARY_TYPE STATIC)
endif()

include(${GKLIB_PATH}/GKlibSystem.cmake)

# List of paths that the compiler will search for header files.
# i.e., the -I equivalent
include_directories(include)
include_directories(${MPI_INCLUDE_PATH})
include_directories(${GKLIB_PATH})
include_directories(${METIS_PATH}/include)

# List of directories that cmake will look for CMakeLists.txt
add_subdirectory(${METIS_PATH}/libmetis ${CMAKE_BINARY_DIR}/libmetis)
add_subdirectory(include)
add_subdirectory(libparmetis)
add_subdirectory(programs)

# This is for testing during development and is not being distributed
#add_subdirectory(test)
