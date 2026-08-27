function(check_git_submodule DIR)
	# Resolve relative to the caller's source list instead of the process working
	# directory. Script-mode invocations (the installer) may run from the repo
	# root, where a same-named submodule directory does not exist.
    file(GLOB DIR_FILES "${CMAKE_CURRENT_LIST_DIR}/${DIR}/*")
	if (DIR_FILES STREQUAL "")
		message(WARNING "It looks like the '${DIR}' directory is empty. Did you forget to update git submodules?")
		find_package(Git MODULE)
		if (Git_FOUND)
			message(STATUS "Updating the '${DIR}' git submodule to fix.")
			execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${CMAKE_CURRENT_LIST_DIR}" submodule update --init -- "${DIR}")
		endif()
	endif()
endfunction()
