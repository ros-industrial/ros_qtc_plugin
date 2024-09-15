# package metadata
set(CPACK_PACKAGE_NAME "qtcreator-plugin-ros")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Qt Creator plugin for ROS workspaces")
set(CPACK_PACKAGE_DESCRIPTION
"The ROS Qt Creator Plug-in is developed specifically for ROS to increase a
developers' efficiency by simplifying tasks and creating a centralized location
for ROS tools. Since it is built on top of the Qt Creator platform, users have
access to all of its existing features like: syntax highlighting, code
indexing, editors (C++ , Python, etc.), code completion, version control (Git,
Subversion, etc.), debuggers (GDB, CDB, LLDB, etc.), and much more."
)
set(CPACK_INSTALL_DEFAULT_DIRECTORY_PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE
)

# only generate the plugin archive by default
set(CPACK_GENERATOR "ZIP")
set(CPACK_PROJECT_CONFIG_FILE "${CMAKE_SOURCE_DIR}/packaging/cpack-config.cmake")

# Debian package settings
math(EXPR PROJECT_VERSION_MAJOR_NEXT "${PROJECT_VERSION_MAJOR}+1")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Christian Rauch <Rauch.Christian@gmx.de>")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS YES)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "qtcreator (>=${PROJECT_VERSION_MAJOR}), qtcreator (<<${PROJECT_VERSION_MAJOR_NEXT})")
set(CPACK_DEBIAN_DEBUGINFO_PACKAGE ON)

include(CPack)
