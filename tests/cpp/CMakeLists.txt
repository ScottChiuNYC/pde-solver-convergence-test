cmake_minimum_required(VERSION 3.25)
project(pde_software_replication_core_test)

find_package(GTest CONFIG REQUIRED)

# Add all .cpp files in this directory as test sources
file(GLOB TEST_SOURCES *.cpp)
add_executable(pde_software_replication_core_test ${TEST_SOURCES})

target_include_directories(pde_software_replication_core_test PRIVATE
    ${CMAKE_SOURCE_DIR}/cpp/include
)

target_link_libraries(pde_software_replication_core_test PRIVATE
    pde_software_replication_core_static
    GTest::gtest
    GTest::gtest_main
    # GTest::gmock
    # GTest::gmock_main
)

add_test(NAME pde_software_replication_core_test COMMAND pde_software_replication_core_test)
