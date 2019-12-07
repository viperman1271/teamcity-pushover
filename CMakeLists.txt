cmake_minimum_required (VERSION 3.5)
project (teamcity-pushover)

add_executable(teamcity-pushover source/main.cpp)

include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR})

if(WIN32)
    add_definitions(-D_WINSOCK_DEPRECATED_NO_WARNINGS -DPLATFORM_WINDOWS)
endif()

if(UNIX)
    include(~/vcpkg/scripts/buildsystems/vcpkg.cmake)
    
    add_definitions(-DPLATFORM_UNIX)
    
    set_property(TARGET teamcity-pushover PROPERTY CXX_STANDARD 11)
    
    find_package(CLI11 CONFIG REQUIRED)
    find_package(CURL REQUIRED)
    find_package(yaml-cpp CONFIG REQUIRED)
    find_package(json-c CONFIG REQUIRED)
    
    target_link_libraries(teamcity-pushover PRIVATE ${CURL_LIBRARIES} yaml-cpp json-c::json-c)
    target_include_directories(teamcity-pushover PRIVATE ${CURL_INCLUDE_DIRS})
endif()
