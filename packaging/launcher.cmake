execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy /opt/qt-creator/share/applications/org.qt-project.qtcreator.desktop ${CMAKE_CURRENT_BINARY_DIR}/usr/share/applications/qtcreator-ros.desktop
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/qtcreator-ros.svg ${CMAKE_CURRENT_BINARY_DIR}/usr/share/icons/hicolor/qtcreator-ros.svg
    COMMAND_ERROR_IS_FATAL ANY
)

execute_process(
    COMMAND desktop-file-edit "${CMAKE_CURRENT_BINARY_DIR}/usr/share/applications/qtcreator-ros.desktop" "--set-key=Exec" "--set-value=/opt/qt-creator/bin/qtcreator %F" "--set-key=Name" "--set-value=Qt Creator ROS" "--set-key=Icon" "--set-value=/usr/share/icons/hicolor/qtcreator-ros.svg"
    COMMAND_ERROR_IS_FATAL ANY
)

execute_process(
    COMMAND desktop-file-validate "${CMAKE_CURRENT_BINARY_DIR}/usr/share/applications/qtcreator-ros.desktop"
    COMMAND_ERROR_IS_FATAL ANY
)
