cmake_minimum_required(VERSION 3.20)
project(prestosynth VERSION 0.1.0 LANGUAGES CXX C)

option(BUILD_TESTING "Avoid libsamplerate to build test." OFF)

#set(CMAKE_BUILD_TYPE Release)

#if(${CMAKE_BUILD_TYPE} EQUAL "Debug")
#	message(STATUS "Debug building")
#	add_compile_options(-fno-inline -g)
#endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/mio)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libsamplerate)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/MPMCQueue)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/eigen)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/gcem)

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
target_link_libraries(prestosynth eigen)
target_link_libraries(prestosynth MPMCQueue)
target_link_libraries(prestosynth gcem)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
	# Examples
	add_executable(readsfinfo ${CMAKE_CURRENT_SOURCE_DIR}/example/readsfinfo.cpp)
	target_link_libraries(readsfinfo PRIVATE prestosynth)

	add_executable(fetchsample ${CMAKE_CURRENT_SOURCE_DIR}/example/fetchsample.cpp)
	target_link_libraries(fetchsample PRIVATE prestosynth)

	add_executable(buildnote ${CMAKE_CURRENT_SOURCE_DIR}/example/buildnote.cpp)
	target_link_libraries(buildnote PRIVATE prestosynth)

	add_executable(render ${CMAKE_CURRENT_SOURCE_DIR}/example/render.cpp)
	target_link_libraries(render PRIVATE prestosynth)

	# Tests
	add_executable(test_conv ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_conv.cpp)
	target_link_libraries(test_conv PRIVATE prestosynth)

	add_executable(test_filter ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_filter.cpp)
	target_link_libraries(test_filter PRIVATE prestosynth)
endif()
