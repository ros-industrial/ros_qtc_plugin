cmake_minimum_required(VERSION 3.10)

project(ROSProjectManager VERSION 14.2)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Werror -Wall -Wextra -Wpedantic -Wsuggest-override)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    add_link_options("-Wl,-z,relro,-z,now,-z,defs")
endif()

option(BUILD_ROSTERMINAL "build with an integrated ROS terminal")

add_subdirectory(src/project_manager)

include(packaging/cpack.cmake)
