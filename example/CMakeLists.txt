cmake_minimum_required(VERSION 3.8)

project(picogl_example)

set(CMAKE_CXX_STANDARD 17)
#message("Your C++ compiler supports these C++ features:")
#foreach(i ${CMAKE_CXX_COMPILE_FEATURES})
#  message("${i}")
#endforeach()

#set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

if (MSVC)
	add_definitions(/std:c++17 /Zc:preprocessor)
endif(MSVC)

# picoGL
set(PICOGL_PATH "../src/")
set(PICOGL_FRAMEWORK "${PICOGL_PATH}/framework/")
set(PICOGL_FRAMEWORK_INCLUDE "${PICOGL_FRAMEWORK}/include/")
set(PICOGL_FRAMEWORK_SHADER_PATH "${PICOGL_FRAMEWORK}/shaders/")

file(GLOB PICOGL_FILE "${PICOGL_PATH}/picogl.hpp")
file(GLOB PICOGL_FRAMEWORK_HEADERS "${PICOGL_FRAMEWORK_INCLUDE}/*.h")
file(GLOB PICOGL_FRAMEWORK_SOURCES "${PICOGL_FRAMEWORK}/src/*.cpp")
file(GLOB PICOGL_FRAMEWORK_SHADERS 
	"${PICOGL_FRAMEWORK_SHADER_PATH}/*.vert"
	"${PICOGL_FRAMEWORK_SHADER_PATH}/*.tcs"
	"${PICOGL_FRAMEWORK_SHADER_PATH}/*.tev"
	"${PICOGL_FRAMEWORK_SHADER_PATH}/*.geom"
	"${PICOGL_FRAMEWORK_SHADER_PATH}/*.frag"
	"${PICOGL_FRAMEWORK_SHADER_PATH}/*.comp"
)

## Deps
set(DEPS_PATH "./deps/")

# ImGui
set(IMGUI_PATH "${DEPS_PATH}/imgui/")
file(GLOB IMGUI_HEADERS 
	"${IMGUI_PATH}/*.h"
	"${IMGUI_PATH}/backends/imgui_impl_glfw.h"
	"${IMGUI_PATH}/backends/imgui_impl_opengl3.h")
file(GLOB IMGUI_SRC 
	"${IMGUI_PATH}/*.cpp"
	"${IMGUI_PATH}/backends/imgui_impl_glfw.cpp"
	"${IMGUI_PATH}/backends/imgui_impl_opengl3.cpp")

# GLAD
set(GLAD_PATH "${DEPS_PATH}/glad")
set(GLAD_INCLUDE "${GLAD_PATH}/include/")
file(GLOB GLAD_SRC "${GLAD_PATH}/src/glad.c")

# GLM
set(GLM_INCLUDE "${DEPS_PATH}/glm/")

# GLFW
set(GLFW_PATH "${DEPS_PATH}/glfw/")
set(GLFW_INCLUDE "${DEPS_PATH}/include/")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
set(GLFW_INSTALL OFF CACHE BOOL "")
set(GLFW_DOCUMENT_INTERNALS OFF CACHE BOOL "")
add_subdirectory(${GLFW_PATH} "${GLFW_PATH}/build/")

#SPDLOG
set(SPDLOG_PATH "${DEPS_PATH}/spdlog/")
set(SPDLOG_INCLUDE "${SPDLOG_PATH}/include/")
add_subdirectory(${SPDLOG_PATH} "${SPDLOG_PATH}/build/")

#STBIMAGE
set(STB_PATH "${DEPS_PATH}/stb/")
set(STB_IMAGE_SRC "${STB_PATH}/stb_image.h")

#TINYOBJ
set(TINYOBJ_PATH "${DEPS_PATH}/tinyobjloader/")
set(TINYOBJ_INCLUDE "${TINYOBJ_PATH}")
set(TINYOBJ_SRC "${TINYOBJ_PATH}/tiny_obj_loader.h/")

source_group("Src" FILES 
	main.cpp
	${PICOGL_FILE}
	${PICOGL_FRAMEWORK_HEADERS}
	${PICOGL_FRAMEWORK_SOURCES}

)

add_executable(picogl_example 
	main.cpp
	${PICOGL_FILE}
	${PICOGL_FRAMEWORK_HEADERS}
	${PICOGL_FRAMEWORK_SOURCES}
	${GLAD_SRC}
	${IMGUI_HEADERS}
	${IMGUI_SRC}
	${TINYOBJ_SRC}
	${STB_IMAGE_SRC}
)

target_include_directories(picogl_example PRIVATE 
	${PICOGL_PATH}
	${PICOGL_FRAMEWORK_INCLUDE}
	${GLAD_INCLUDE}
	${IMGUI_PATH}
	${GLFW_INCLUDE}
	${GLM_INCLUDE}
	${SPDLOG_INCLUDE}
	${TINYOBJ_INCLUDE}
	${STB_PATH}
)

target_link_libraries(picogl_example PRIVATE
	glfw
	spdlog
)