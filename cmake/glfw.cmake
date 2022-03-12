if(TARGET glfw::glfw)
    return()
endif()

message(STATUS "Creating target 'glfw::glfw'")

include(FetchContent)

FetchContent_Declare(
	glfw
	GIT_REPOSITORY "https://github.com/glfw/glfw"
	GIT_TAG "1461c59aa2426b25503102e62bc8f4b65e079c5f"
)

FetchContent_MakeAvailable(glfw)
	
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE INTERNAL "Generate installation target")

add_library(glfw::glfw ALIAS glfw)
