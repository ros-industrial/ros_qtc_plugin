TEMPLATE = aux

include(../ros_qtc_plugin.pri)

STATIC_BASE = $$PWD
STATIC_OUTPUT_BASE = $$IDE_DATA_PATH
STATIC_INSTALL_BASE = $$INSTALL_DATA_PATH

DATA_DIRS = \
    templates \
    styles \
    qtermwidget

for(data_dir, DATA_DIRS) {
    files = $$files($$PWD/$$data_dir/*, true)
    # Info.plist.in are handled below
    for(file, files):!contains(file, ".*/Info\\.plist\\.in$"):!exists($$file/*): \
        STATIC_FILES += $$file
}

include($$QTCREATOR_SOURCES/qtcreatordata.pri)

#TEMPLATE = app
#TARGET = phony_target
#CONFIG -= qt sdk separate_debug_info gdb_dwarf_index
#QT =
#LIBS =
#QMAKE_LINK = @: IGNORE THIS LINE
#OBJECTS_DIR =

#STATIC_BASE = $$PWD

## files/folders that are conditionally "deployed" to the build directory
#DATA_DIRS = \
#    templates \
#    styles

#for(data_dir, DATA_DIRS) {
#    files = $$files($$PWD/$$data_dir/*, true)
#    # Info.plist.in are handled below
#    for(file, files):!contains(file, ".*/Info\\.plist\\.in$"):!exists($$file/*): \
#        STATIC_FILES += $$file
#}

## used in custom compilers which just copy files
#defineReplace(stripStaticBase) {
#    return($$relative_path($$1, $$STATIC_BASE))
#}

## handle copying of static files to the build directory
#!isEmpty(STATIC_FILES) {
#    isEmpty(STATIC_BASE): \
#        error("Using STATIC_FILES without having STATIC_BASE set")

#    copy2build.input += STATIC_FILES
#    copy2build.output = $$IDE_DATA_PATH/${QMAKE_FUNC_FILE_IN_stripStaticBase}
#    copy2build.variable_out = PRE_TARGETDEPS
#    copy2build.commands = $$QMAKE_COPY ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
#    copy2build.name = COPY ${QMAKE_FILE_IN}
#    copy2build.config += no_link
#    QMAKE_EXTRA_COMPILERS += copy2build

#    !osx {
#        static.files = $$STATIC_FILES
#        static.base = $$STATIC_BASE
#        static.path = $$INSTALL_DATA_PATH
#        INSTALLS += static
#    }
#}


