cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project(signal-handler-test)

include(EnableWarnings)
include(DefaultBuildRelease)

find_package(Catch2 REQUIRED)

add_executable(
  sgnl-test
  EXCLUDE_FROM_ALL
  "${PROJECT_SOURCE_DIR}/test.cpp")
enable_warnings(sgnl-test PRIVATE)
target_link_libraries(sgnl-test sgnl::sgnl Catch2::Catch2 "-fsanitize=thread")
target_compile_features(sgnl-test PUBLIC cxx_std_17)
target_compile_options(sgnl-test PUBLIC "-fsanitize=thread")

