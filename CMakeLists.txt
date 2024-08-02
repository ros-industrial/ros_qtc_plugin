cmake_minimum_required(VERSION 3.10)

project(ROSProjectManager VERSION 14.1)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Werror -Wall -Wextra -Wpedantic -Wsuggest-override)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    add_link_options("-Wl,-z,relro,-z,now,-z,defs")
endif()

option(BUILD_ROSTERMINAL "build with an integrated ROS terminal")

add_subdirectory(src/project_manager)

# generate plugin archive
set(CPACK_GENERATOR "ZIP")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
include(CPack)
