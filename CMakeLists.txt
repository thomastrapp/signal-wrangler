cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project(signal-wrangler VERSION 0.4.0)

# Add the top-level cmake module directory to CMAKE_MODULE_PATH
list(INSERT CMAKE_MODULE_PATH 0 ${PROJECT_SOURCE_DIR}/cmake)

find_package(Threads REQUIRED)
if( NOT CMAKE_USE_PTHREADS_INIT )
  message(FATAL_ERROR "pthreads required")
endif()

add_library(sgnl INTERFACE)
add_library(sgnl::sgnl ALIAS sgnl)
target_include_directories(
  sgnl INTERFACE
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>)
target_link_libraries(sgnl INTERFACE Threads::Threads)
target_compile_features(sgnl INTERFACE cxx_std_17)

include(EnableWarnings)
enable_warnings(sgnl INTERFACE)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/SgnlConfigVersion.cmake"
  COMPATIBILITY SameMajorVersion)

include(GNUInstallDirs)
install(TARGETS sgnl
        EXPORT SgnlTargets)
install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/sgnl"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h")
install(FILES "${PROJECT_SOURCE_DIR}/cmake/SgnlConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/SgnlConfigVersion.cmake"
        DESTINATION lib/cmake/sgnl)
install(EXPORT SgnlTargets
        FILE SgnlTargets.cmake
        NAMESPACE sgnl::
        DESTINATION lib/cmake/sgnl)

add_subdirectory("test" EXCLUDE_FROM_ALL)

add_executable(example EXCLUDE_FROM_ALL "example/example.cpp")
target_link_libraries(example sgnl::sgnl)

