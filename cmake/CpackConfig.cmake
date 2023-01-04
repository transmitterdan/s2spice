# installer rules. 
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PACKAGE_NAME} installation.")
set(CPACK_PACKAGE_VENDOR "transmitterdan")
set(CPACK_PACKAGE_VERSION ${PROJECT_VER})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VER_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VER_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VER_PATCH})
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL BOOL:ON)
set(CPACK_NSIS_MODIFY_PATH BOOL::ON)

# if you have an icon set the path here 
SET(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}\\\\${PACKAGE_NAME}.ico")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_VENDOR}\\\\${PACKAGE_NAME}")
set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\s2spice.exe")
set(CPACK_NSIS_DISPLAY_NAME "${PACKAGE_NAME} ${PROJECT_VER}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/license.md")
set(CPACK_NSIS_MUI_FINISHPAGE_RUN "s2spice.exe")
set(CPACK_NSIS_CREATE_ICONS_EXTRA
    "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\S2spice.lnk' '$INSTDIR\\\\bin\\\\s2spice.exe'"
)
set(CPACK_NSIS_DELETE_ICONS_EXTRA
    "Delete '$SMPROGRAMS\\\\$START_MENU\\\\S2spice.lnk'"
)
include(CPackComponent)

cpack_add_install_type(Full DISPLAY_NAME "Install Everything")
cpack_add_install_type(Upgrade DISPLAY_NAME "Upgrade Only")

cpack_add_component(binaries
    DISPLAY_NAME "${PACKAGE_NAME} application"
    DESCRIPTION "This will install the ${PACKAGE_NAME} application."
    REQUIRED
    INSTALL_TYPES Full Upgrade
)

cpack_add_component(data_files
    DISPLAY_NAME "Data files"
    DESCRIPTION "Data files for ${PACKAGE_NAME}."
    INSTALL_TYPES Full
)

set(CPACK_COMPONENTS_ALL binaries data_files)
if (CMAKE_CL_64)
    set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
else (CMAKE_CL_64)
    set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
endif (CMAKE_CL_64)
include(CPack)
