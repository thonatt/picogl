if(TARGET stb)
    return()
endif()

cmake_minimum_required(VERSION 3.18)
message(STATUS "Creating 'stb' targets")

include(FetchContent)

FetchContent_Declare(
	stb
	GIT_REPOSITORY "https://github.com/nothings/stb.git"
	GIT_TAG "af1a5bc352164740c1cc1354942b1c6b72eacb8a"
)

FetchContent_MakeAvailable(stb)

if(USE_STB_IMAGE)
	message(STATUS "Creating target 'stb::image'")
	file(GLOB STB_IMAGE_FILE "${stb_SOURCE_DIR}/stb_image.h")
	set_source_files_properties(
		${STB_IMAGE_FILE} PROPERTIES
		LANGUAGE CXX
	)

	add_library(stb_image STATIC ${STB_IMAGE_FILE})
	target_include_directories(stb_image PUBLIC ${stb_SOURCE_DIR})
	target_compile_definitions(stb_image PRIVATE STB_IMAGE_IMPLEMENTATION)

	add_library(stb::image ALIAS stb_image)
endif()