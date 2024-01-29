cmake_minimum_required(VERSION 3.20)
project(prestosynth VERSION 0.1.0 LANGUAGES CXX C)

option(BUILD_EXAMPLES "Build examples" OFF)

# option(BUILD_TESTING "Avoid libsamplerate to build test." OFF)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/mio)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/MPMCQueue)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libsamplerate)

file(GLOB_RECURSE prestosynth_src src/*.cpp src/*.c)

foreach(src_file ${prestosynth_src})
    message(STATUS "prestosynth_src: ${src_file}")
endforeach()

add_library(prestosynth ${prestosynth_src})
add_library(prestosynth::prestosynth ALIAS prestosynth)

target_compile_features(prestosynth PUBLIC cxx_std_17)
target_include_directories(prestosynth PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(prestosynth mio)
target_link_libraries(prestosynth samplerate)

if(BUILD_EXAMPLES)
	add_executable(readsfinfo ${CMAKE_CURRENT_SOURCE_DIR}/example/readsfinfo.cpp)
	target_link_libraries(readsfinfo PRIVATE prestosynth)

	add_executable(fetchsample ${CMAKE_CURRENT_SOURCE_DIR}/example/fetchsample.cpp)
	target_link_libraries(fetchsample PRIVATE prestosynth)

	add_executable(buildsfidx ${CMAKE_CURRENT_SOURCE_DIR}/example/buildsfidx.cpp)
	target_link_libraries(buildsfidx PRIVATE prestosynth)
endif()