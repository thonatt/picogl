if(TARGET spdlog::spdlog)
    return()
endif()

message(STATUS "Creating target 'spdlog::spdlog'")

include(FetchContent)

FetchContent_Declare(
	spdlog
	GIT_REPOSITORY "https://github.com/gabime/spdlog"
	GIT_TAG "b1478d98f017f3a7644e6e3a16fab6a47a5c26ba"
)

FetchContent_MakeAvailable(spdlog)