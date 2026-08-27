# Script-mode CMake does not search this script's directory for modules.
# Use an absolute path so local and CI invocations behave the same way.
include("${CMAKE_CURRENT_LIST_DIR}/check_git_submodule.cmake")
check_git_submodule(dechamps_CMakeUtils)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/dechamps_CMakeUtils")
# Winget installs Inno Setup per-user by default.  Prefer that explicit path
# when present; the bundled finder continues to cover machine-wide installs.
if(NOT DEFINED InnoSetup_iscc_EXECUTABLE
   AND EXISTS "$ENV{LOCALAPPDATA}/Programs/Inno Setup 6/ISCC.exe")
    set(InnoSetup_iscc_EXECUTABLE "$ENV{LOCALAPPDATA}/Programs/Inno Setup 6/ISCC.exe")
endif()
find_package(InnoSetup MODULE REQUIRED)

find_package(Git MODULE REQUIRED)
set(DECHAMPS_CMAKEUTILS_GIT_DIR "${CMAKE_CURRENT_LIST_DIR}/flexasio")
include(version/version)
if(NOT DEFINED DISTRIBUTION_VERSION)
    set(DISTRIBUTION_VERSION "0.1.0")
endif()
set(FLEXASIO_VERSION "${DISTRIBUTION_VERSION}")

configure_file("${CMAKE_CURRENT_LIST_DIR}/installer.in.iss" "${CMAKE_CURRENT_LIST_DIR}/out/installer.iss" @ONLY)
include(execute_process_or_die)
execute_process_or_die(
    COMMAND "${InnoSetup_iscc_EXECUTABLE}"
        "${CMAKE_CURRENT_LIST_DIR}/out/installer.iss"
        "/O${CMAKE_CURRENT_LIST_DIR}/out/installer"
        "/FFlexASIO-GUI-Tester-${FLEXASIO_VERSION}-Setup"
)
