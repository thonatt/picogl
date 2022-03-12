if(TARGET glad::glad)
    return()
endif()

message(STATUS "Creating target 'glad::glad'")

include(FetchContent)

FetchContent_Declare(
	glad
	GIT_REPOSITORY "https://github.com/Dav1dde/glad"
	GIT_TAG "v0.1.36"
)

FetchContent_MakeAvailable(glad)

set(GLAD_PROFILE "core" CACHE STRING "OpenGL profile")
set(GLAD_API "gl=" CACHE STRING "API type/version pairs, like \"gl=3.2,gles=\", no version means latest")
set(GLAD_GENERATOR "c" CACHE STRING "Language to generate the binding for")
	
add_library(glad::glad ALIAS glad)

