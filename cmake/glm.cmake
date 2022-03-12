if(TARGET glm::glm)
    return()
endif()

message(STATUS "Creating target 'glm::glm'")

include(FetchContent)

FetchContent_Declare(
	glm
	GIT_REPOSITORY "https://github.com/g-truc/glm"
	GIT_TAG "6ad79aae3eb5bf809c30bf1168171e9e55857e45"
)

FetchContent_MakeAvailable(glm)