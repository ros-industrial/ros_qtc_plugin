cmake_minimum_required(VERSION 3.10)

project(ROSProjectManager VERSION 0.3.11)

add_subdirectory(src/project_manager)

# generate plugin archive
set(CPACK_GENERATOR "ZIP")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
include(CPack)
