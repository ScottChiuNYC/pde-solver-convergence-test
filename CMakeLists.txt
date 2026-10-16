cmake_minimum_required(VERSION 3.25)
project(pde_software_replication VERSION 0.1.0 LANGUAGES CXX)

include(CTest)
option(BUILD_PYTHON_BINDINGS "Build the pybind11 extension" ON)
option(BUILD_EXAMPLES "Build example programs" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(cpp)
if(BUILD_EXAMPLES)
    add_subdirectory(examples/cpp)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests/cpp)
endif()
