cmake_minimum_required(VERSION 3.11)

message(STATUS "Creating target 'picogl::picogl'")

project(picogl LANGUAGES CXX)

set(PICOGL_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include")
file(GLOB PICOGL_HEADER "${PICOGL_INCLUDE_PATH}/picogl/picogl.hpp")

add_library(picogl INTERFACE)
target_include_directories(picogl INTERFACE ${PICOGL_INCLUDE_PATH})
target_sources(picogl INTERFACE ${PICOGL_HEADER})
add_library(picogl::picogl ALIAS picogl)

if(PICOGL_USE_FRAMEWORK)

	message(STATUS "Creating target 'picogl::framework'")
	include(cmake/glad.cmake)
	include(cmake/glfw.cmake)
	include(cmake/glm.cmake)
	include(cmake/imgui.cmake)
	include(cmake/spdlog.cmake)
	set(USE_STB_IMAGE ON)
	include(cmake/stb.cmake)
	include(cmake/tinyobjloader.cmake)

	set(PICOGL_FRAMEWORK_SRC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src/framework/")
	set(PICOGL_FRAMEWORK_SHADER_PATH "${PICOGL_FRAMEWORK_SRC_PATH}/shaders/")
	
	file(GLOB PICOGL_FRAMEWORK_HEADERS "${PICOGL_INCLUDE_PATH}/picogl/framework/*.h")
	file(GLOB PICOGL_FRAMEWORK_SOURCES "${PICOGL_FRAMEWORK_SRC_PATH}/*.cpp")
	file(GLOB PICOGL_FRAMEWORK_SHADERS 
		"${PICOGL_FRAMEWORK_SHADER_PATH}/*.vert"
		"${PICOGL_FRAMEWORK_SHADER_PATH}/*.tcs"
		"${PICOGL_FRAMEWORK_SHADER_PATH}/*.tev"
		"${PICOGL_FRAMEWORK_SHADER_PATH}/*.geom"
		"${PICOGL_FRAMEWORK_SHADER_PATH}/*.frag"
		"${PICOGL_FRAMEWORK_SHADER_PATH}/*.comp"
	)
	source_group("shaders/" FILES ${PICOGL_FRAMEWORK_SHADERS})
	
	add_library(picogl_framework STATIC
		${PICOGL_FRAMEWORK_HEADERS}
		${PICOGL_FRAMEWORK_SOURCES}
		${PICOGL_FRAMEWORK_SHADERS}
	)
	target_link_libraries(picogl_framework 
		PUBLIC 
			picogl::picogl
			glad::glad
			glfw::glfw
			glm::glm
			imgui::imgui
			spdlog::spdlog
			stb::image
			tinyobjloader
	)
	add_library(picogl::framework ALIAS picogl_framework)
	
endif()
