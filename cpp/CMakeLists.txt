cmake_minimum_required(VERSION 3.25)
project(pde_software_replication_core VERSION ${PROJECT_VERSION} LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_library(pde_software_replication_core_static STATIC ${CORE_SOURCES})
set_target_properties(
    pde_software_replication_core_static
    PROPERTIES POSITION_INDEPENDENT_CODE ON EXPORT_NAME core
)

target_include_directories(
    pde_software_replication_core_static
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(pde_software_replication_core_static PUBLIC cxx_std_17)

install(TARGETS pde_software_replication_core_static EXPORT pde_software_replication_coreTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
set(package_dir "${CMAKE_INSTALL_LIBDIR}/cmake/pde_software_replication_core")
configure_package_config_file("${CMAKE_SOURCE_DIR}/cmake/LibraryConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/pde_software_replication_coreConfig.cmake"
    INSTALL_DESTINATION "${package_dir}"
)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/pde_software_replication_coreConfigVersion.cmake"
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion
)
install(EXPORT pde_software_replication_coreTargets NAMESPACE pde_software_replication_core:: DESTINATION "${package_dir}")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/pde_software_replication_coreConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/pde_software_replication_coreConfigVersion.cmake" DESTINATION "${package_dir}"
)

if(BUILD_PYTHON_BINDINGS)

include(FetchContent)
FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG v3.0.1
)
FetchContent_MakeAvailable(pybind11)

set(PYBIND11_FINDPYTHON ON)
pybind11_add_module(pde_software_replication_core bindings/hello_bindings.cpp)
target_link_libraries(pde_software_replication_core PRIVATE pde_software_replication_core_static)

# Put the extension directly beside the Python package sources on every
# platform and for every multi-config build configuration. This avoids
# generator-specific copy logic in build.py.
set(PYTHON_PACKAGE_DIR "${CMAKE_SOURCE_DIR}/src/pde_software_replication")
set_target_properties(
    pde_software_replication_core
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${PYTHON_PACKAGE_DIR}"
        RUNTIME_OUTPUT_DIRECTORY "${PYTHON_PACKAGE_DIR}"
)

foreach(config Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${config}" config_upper)
    set_target_properties(
        pde_software_replication_core
        PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY_${config_upper} "${PYTHON_PACKAGE_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_${config_upper} "${PYTHON_PACKAGE_DIR}"
    )
endforeach()
endif()
