if(TARGET imgui::imgui)
    return()
endif()

message(STATUS "Creating target 'imgui::imgui'")

include(FetchContent)

FetchContent_Declare(
	imgui
	GIT_REPOSITORY "https://github.com/ocornut/imgui"
	GIT_TAG "v1.87"
)

FetchContent_MakeAvailable(imgui)

file(GLOB IMGUI_HEADERS 
	"${imgui_SOURCE_DIR}/*.h"
	"${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.h"
	"${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.h")
file(GLOB IMGUI_SRC 
	"${imgui_SOURCE_DIR}/*.cpp"
	"${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
	"${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp")
	
add_library(imgui STATIC ${IMGUI_HEADERS} ${IMGUI_SRC})
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
target_link_libraries(imgui PUBLIC glfw::glfw)

add_library(imgui::imgui ALIAS imgui)
