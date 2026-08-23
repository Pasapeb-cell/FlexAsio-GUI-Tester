find_package(Qt6 CONFIG REQUIRED COMPONENTS Widgets)

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
target_compile_options(FlexASIOGUI PRIVATE /WX- /W3 /analyze-)
target_link_libraries(FlexASIOGUI PRIVATE
    Qt6::Widgets
    FlexASIO_config
    FlexASIOUtil_shell
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

# windeployqt drops its output (DLLs and plugin subdirectories) next to the built exe, not
# into the install tree - carry the whole directory over so the installed exe is runnable
# on its own.
install(DIRECTORY "$<TARGET_FILE_DIR:FlexASIOGUI>/" DESTINATION bin)
