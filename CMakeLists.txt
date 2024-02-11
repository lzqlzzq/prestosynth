cmake_minimum_required(VERSION 3.20)
project(prestosynth VERSION 0.1.0 LANGUAGES CXX C)

option(BUILD_EXAMPLES "Build examples" OFF)
option(BUILD_TESTING "Avoid libsamplerate to build test." OFF)

set(CMAKE_BUILD_TYPE Release)
# set(CMAKE_BUILD_TYPE Debug)

if(${CMAKE_BUILD_TYPE} EQUAL "Debug")
	message(STATUS "Debug building")
	add_compile_options(-fno-inline -g)
endif()

# set(XTENSOR_USE_XSIMD 1)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/mio)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/MPMCQueue)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libsamplerate)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/xtl)
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/xsimd)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/xtensor)

file(GLOB_RECURSE prestosynth_src src/*.cpp src/*.c)

foreach(src_file ${prestosynth_src})
    message(STATUS "prestosynth_src: ${src_file}")
endforeach()

add_library(prestosynth ${prestosynth_src})
add_library(prestosynth::prestosynth ALIAS prestosynth)

target_compile_features(prestosynth PRIVATE cxx_std_17)
target_include_directories(prestosynth PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(prestosynth mio)
target_link_libraries(prestosynth samplerate)
target_link_libraries(prestosynth xtensor)

if(BUILD_EXAMPLES)
	add_executable(readsfinfo ${CMAKE_CURRENT_SOURCE_DIR}/example/readsfinfo.cpp)
	target_link_libraries(readsfinfo PRIVATE prestosynth)

	add_executable(fetchsample ${CMAKE_CURRENT_SOURCE_DIR}/example/fetchsample.cpp)
	target_link_libraries(fetchsample PRIVATE prestosynth)

	add_executable(render ${CMAKE_CURRENT_SOURCE_DIR}/example/render.cpp)
	target_link_libraries(render PRIVATE prestosynth)
endif()
