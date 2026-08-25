# Creates the portable preview asset from an already-installed x64 build.
# It deliberately copies an allowlist rather than an entire build directory.
if(NOT DEFINED X64_INSTALL_DIR OR NOT DEFINED OUTPUT_DIR OR NOT DEFINED DISTRIBUTION_VERSION)
  message(FATAL_ERROR "Set X64_INSTALL_DIR, OUTPUT_DIR, and DISTRIBUTION_VERSION")
endif()

set(BIN "${X64_INSTALL_DIR}/bin")
set(NAME "FlexASIO-GUI-Tester-${DISTRIBUTION_VERSION}-Windows-x64")
set(ROOT "${OUTPUT_DIR}/${NAME}")
file(MAKE_DIRECTORY "${ROOT}" "${ROOT}/platforms" "${ROOT}/licenses")

function(copy_required SOURCE DESTINATION)
  if(NOT EXISTS "${SOURCE}")
    message(FATAL_ERROR "Required release file is missing: ${SOURCE}")
  endif()
  file(COPY_FILE "${SOURCE}" "${DESTINATION}" ONLY_IF_DIFFERENT)
endfunction()

foreach(FILE FlexASIOGUI.exe PortAudioDevices.exe portaudio.dll Qt6Core.dll Qt6Gui.dll Qt6Widgets.dll)
  copy_required("${BIN}/${FILE}" "${ROOT}/${FILE}")
endforeach()
copy_required("${BIN}/platforms/qwindows.dll" "${ROOT}/platforms/qwindows.dll")
foreach(FILE README.md RELEASE_NOTES.md THIRD_PARTY_NOTICES.md LICENSE.txt CONFIGURATION.md)
  copy_required("${CMAKE_CURRENT_LIST_DIR}/../${FILE}" "${ROOT}/${FILE}")
endforeach()
copy_required("${CMAKE_CURRENT_LIST_DIR}/portaudio/LICENSE.txt" "${ROOT}/licenses/PortAudio-LICENSE.txt")
copy_required("${CMAKE_CURRENT_LIST_DIR}/tinytoml/LICENSE" "${ROOT}/licenses/tinytoml-LICENSE.txt")

file(ARCHIVE_CREATE OUTPUT "${OUTPUT_DIR}/${NAME}.zip" PATHS "${NAME}" FORMAT zip WORKING_DIRECTORY "${OUTPUT_DIR}")
file(SHA256 "${OUTPUT_DIR}/${NAME}.zip" HASH)
file(WRITE "${OUTPUT_DIR}/SHA256SUMS.txt" "${HASH}  ${NAME}.zip\n")
