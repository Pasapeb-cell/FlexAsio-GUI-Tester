option(FLEXASIOGUI_ALLOW_UNSUPPORTED_QT "Allow a local development build with an older Qt runtime" OFF)
if(FLEXASIOGUI_ALLOW_UNSUPPORTED_QT)
    message(WARNING "Building with an unsupported Qt runtime. This build must not be distributed as a release.")
    find_package(Qt6 CONFIG REQUIRED COMPONENTS Widgets)
else()
    find_package(Qt6 6.11.2 CONFIG REQUIRED COMPONENTS Widgets)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_executable(FlexASIOGUI WIN32
    main.cpp
    core/config_io.cpp
    core/device_enumerator.cpp
    core/latency_calculator.cpp
    audio/signal_generator.cpp
    audio/engine.cpp
    audio/auto_tuner.cpp
    ui/theme.cpp
    ui/main_window.cpp
    ui/settings_tab.cpp
    ui/latency_tab.cpp
    ui/tester_tab.cpp
    ui/widgets/quality_indicator.cpp
    ui/widgets/level_meter.cpp
    ui/widgets/latency_pipeline.cpp
    ui/widgets/angular_panel.cpp
    ui/widgets/angular_button.cpp
    resources/resources.qrc
    flexasiogui.rc
)
target_compile_definitions(FlexASIOGUI PRIVATE PROJECT_DESCRIPTION="FlexASIO GUI Tester")
# The parent CMakeLists.txt sets /WX /W4 /analyze for the whole flexasio tree, tuned for
# FlexASIO's own driver code. Qt headers and MOC-generated code aren't vetted against that
# bar, so relax it here (later flags win over earlier ones on the MSVC command line).
target_compile_options(FlexASIOGUI PRIVATE /WX- /W3 /analyze- /guard:cf /sdl)
target_link_options(FlexASIOGUI PRIVATE /guard:cf /DYNAMICBASE /NXCOMPAT /HIGHENTROPYVA /CETCOMPAT)
target_link_libraries(FlexASIOGUI PRIVATE
    Qt6::Widgets
    FlexASIO_config
    FlexASIOUtil_shell
	FlexASIOUtil_portaudio
    PortAudio::PortAudio
)
install(TARGETS FlexASIOGUI RUNTIME DESTINATION bin)

# Qt6::Widgets is a shared library build - the executable needs its DLLs (and the platform
# plugin) alongside it to run. windeployqt is Qt's own tool for gathering exactly those.
find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${Qt6_DIR}/../../../bin")
if(WINDEPLOYQT_EXECUTABLE)
    add_custom_command(TARGET FlexASIOGUI POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations $<TARGET_FILE:FlexASIOGUI>
        COMMENT "Deploying Qt6 runtime dependencies for FlexASIOGUI"
    )
endif()

# Release packaging is allowlisted. Do not install the build directory: it may contain
# PDBs, logs, development plugins, or unrelated DLLs.
get_filename_component(FLEXASIOGUI_QT_PREFIX "${Qt6_DIR}/../../.." ABSOLUTE)
install(FILES
    "${FLEXASIOGUI_QT_PREFIX}/bin/Qt6Core.dll"
    "${FLEXASIOGUI_QT_PREFIX}/bin/Qt6Gui.dll"
    "${FLEXASIOGUI_QT_PREFIX}/bin/Qt6Widgets.dll"
    DESTINATION bin)
install(FILES "${FLEXASIOGUI_QT_PREFIX}/plugins/platforms/qwindows.dll" DESTINATION bin/platforms)

include(CTest)
if(BUILD_TESTING)
    add_executable(FlexASIOGUIConfigTests
        tests/config_io_tests.cpp
        core/config_io.cpp
    )
    target_link_libraries(FlexASIOGUIConfigTests PRIVATE
        Qt6::Core
        FlexASIO_config
        FlexASIOUtil_shell
    )
    add_test(NAME FlexASIOGUIConfigTests COMMAND FlexASIOGUIConfigTests)
endif()
