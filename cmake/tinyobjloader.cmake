if(TARGET tinyobjloader)
    return()
endif()

message(STATUS "Creating target 'tinyobjloader'")

include(FetchContent)

FetchContent_Declare(
	tinyobjloader
	GIT_REPOSITORY "https://github.com/tinyobjloader/tinyobjloader"
	GIT_TAG "v2.0.0rc9"
)

FetchContent_MakeAvailable(tinyobjloader)