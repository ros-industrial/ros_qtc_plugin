# package generator specific settings
if(CPACK_GENERATOR MATCHES "ZIP")
    # generate plugin archive
    set(CPACK_STRIP_FILES YES)
    set(CPACK_PACKAGING_INSTALL_PREFIX "")
elseif(CPACK_GENERATOR MATCHES "DEB")
    # generate Debian package
    set(CPACK_STRIP_FILES NO)
    set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/qt-creator/")
    set(CPACK_INSTALL_SCRIPTS ${CMAKE_CURRENT_LIST_DIR}/launcher.cmake)
endif()
