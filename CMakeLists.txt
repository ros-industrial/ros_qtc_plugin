cmake_minimum_required(VERSION 3.10)
project(LANGUAGES CXX)

# get Qt Creator major version
find_package(QtCreator REQUIRED)
string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)?$" QTC_VER_COMPAT ${IDE_VERSION_COMPAT})
set(QTC_COMPAT_MAJOR ${CMAKE_MATCH_1})
string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)" QTC_VER_DISPLAY ${IDE_VERSION_DISPLAY})
set(QTC_DISPLAY_MAJOR ${CMAKE_MATCH_1})

project(ROSProjectManager VERSION ${QTC_DISPLAY_MAJOR}.0)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Werror -Wall -Wextra -Wpedantic -Wsuggest-override)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    add_link_options("-Wl,-z,relro,-z,now,-z,defs")
endif()

option(BUILD_ROSTERMINAL "build with an integrated ROS terminal")

add_subdirectory(src/project_manager)

include(packaging/cpack.cmake)
