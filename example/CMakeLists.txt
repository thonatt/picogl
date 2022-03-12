cmake_minimum_required(VERSION 3.18)

project(picogl_example)

set(CMAKE_CXX_STANDARD 17)

set(PICOGL_USE_FRAMEWORK ON)
add_subdirectory("../" "/build_picogl/")

add_executable(picogl_example 
	main.cpp
)

target_link_libraries(picogl_example
	PUBLIC
		picogl::framework
)