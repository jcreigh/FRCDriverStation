cmake_minimum_required (VERSION 2.6)
project (narflib)

option(TEST "Build narflib unit tests" ON)
option(STATIC "Use static linking when building executables" ON)
option(STATIC_LIBSTDCXX "Static link libstdc++" OFF)
option(STATIC_LIBGCC "Static link libgcc" OFF)
option(PIE "Create position-independent executables" ON)

set (VERSION_MAJOR 0)
set (VERSION_MINOR 1)
set (VERSION_RELEASE "")

get_directory_property(hasParent PARENT_DIRECTORY)

find_package (ZLIB)
find_package (PNG)

set(NARFLIB_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include)
set(NARFLIB_SOURCE_DIR ${PROJECT_SOURCE_DIR})

configure_file (
	"${PROJECT_SOURCE_DIR}/include/narf/version.h.in"
	"${PROJECT_BINARY_DIR}/include/narf/version.h"
	)

find_package (GTest)
if (NOT GTEST_FOUND AND TEST)
	message(STATUS "GTest not found, disabling building of tests")
	set(TEST OFF)
endif ()

if (hasParent)
	set(STATIC ON)
	set(NARFLIB_INCLUDE_DIRS ${NARFLIB_INCLUDE_DIRS} PARENT_SCOPE)
	set(NARFLIB_SOURCE_DIR ${NARFLIB_SOURCE_DIR} PARENT_SCOPE)
else()

	if (STATIC)
		set(STATIC_LIBSTDCXX ON)
		set(STATIC_LIBGCC ON)
	endif()

	option(PIE "Create position-independent executables" ON)

	if (PIE AND STATIC)
		message(STATUS "Building as static, disabling PIE")
		# PIE + STATIC will result in TEXTRELs unless all libraries have been
		# built for PIC/PIE, so don't allow it
		set(PIE OFF)
	endif()

# -pie with MinGW-w64 breaks startup (incorrect entry point
# is used), so don't allow PIE on Windows for now
# http://sourceforge.net/p/mingw-w64/mailman/mingw-w64-public/thread/CAGFXeQKg0J%2Bax0Dj-cBXGGrwWT15K-aRsWWaBN6wWSDUtRHoLg@mail.gmail.com/
	if (PIE AND WIN32)
		set(PIE OFF)
	endif()
	if (CMAKE_CXX_COMPILER_ID STREQUAL Clang)
		set(CMAKE_COMPILER_IS_CLANG 1)
	endif ()

	if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANG)
		SET(COMMON_C_FLAGS
			"${CMAKE_C_FLAGS} -Wpointer-arith -Wtype-limits -Wwrite-strings -Wuninitialized -Werror=redundant-decls -Wsign-compare -Wconversion -g -fno-ident")

		# don't export symbols (especially for PIC builds, but it doesn't hurt elsewhere)
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")

		if (STATIC)
			SET(STATIC_C_FLAGS "-static")
		endif()

		if (STATIC_LIBGCC)
			SET(STATIC_C_FLAGS "${STATIC_C_FLAGS} -static-libgcc")
		endif()

		if (STATIC_LIBSTDCXX)
			SET(STATIC_C_FLAGS "${STATIC_C_FLAGS} -static-libstdc++")
		endif()

		if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
			# position-independent executable
			if (PIE)
				SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIE")
				SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie -Wl,-z,relro,-z,now")
			endif()

			SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed")
		endif()

		if (NOT STATIC)
			SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
		endif()

		SET(COMMON_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_C_FLAGS} -std=gnu++0x")
	endif ()

	if (MSVC)
		# ask MSVC to define M_PI and related constants
		add_definitions( -D_USE_MATH_DEFINES )
	endif ()
endif()

# generate C source file from binary to embed
FUNCTION(EMBED out_var in_f)
	SET(result)
	SET(src_f_full ${CMAKE_CURRENT_SOURCE_DIR}/${in_f})
	get_filename_component(src_f ${src_f_full} NAME)
	get_filename_component(src_dir ${src_f_full} PATH)
	SET(out_f "${PROJECT_BINARY_DIR}/${src_f}.cpp")
	ADD_CUSTOM_COMMAND(OUTPUT ${out_f}
		COMMAND ${NARFLIB_SOURCE_DIR}/embed.sh ${src_f} ${out_f} #${ZLIB_FOUND}
		DEPENDS ${in_f}
		WORKING_DIRECTORY ${src_dir}
		COMMENT "Embedding ${src_f}"
		VERBATIM
		)
	SET(${out_var} "${out_f}" PARENT_SCOPE)
ENDFUNCTION()

set (NARFLIB_SOURCE_FILES
	bytestream.cpp
	console.cpp
	embed.cpp
	file.cpp
	format.cpp
	ini.cpp
	stdioconsole.cpp
	texteditor.cpp
	tokenize.cpp
	utf.cpp
	net/addr.cpp
	net/socket.cpp
	path.cpp
	)

if (PNG_FOUND)
	list(APPEND NARFLIB_SOURCE_FILES png.cpp)
endif()

include_directories (
	"${NARFLIB_INCLUDE_DIRS}"
	"${PROJECT_BINARY_DIR}/include"
	)

if (ZLIB_FOUND)
	include_directories ("${ZLIB_INCLUDE_DIRS}")
endif()

if(STATIC)
	if (NOT hasParent)
		set(CMAKE_C_FLAGS "${COMMON_C_FLAGS} ${STATIC_C_FLAGS}")
		set(CMAKE_CXX_FLAGS "${COMMON_CXX_FLAGS} ${STATIC_C_FLAGS}")
	endif()
	add_library (narflib STATIC ${NARFLIB_SOURCE_FILES})
else()
	set(CMAKE_C_FLAGS "${COMMON_C_FLAGS}")
	set(CMAKE_CXX_FLAGS "${COMMON_CXX_FLAGS}")
	add_library (narflib SHARED ${NARFLIB_SOURCE_FILES})
	if (ZLIB_FOUND)
		target_link_libraries (narflib ${ZLIB_LIBRARY})
	endif()
	if (PNG_FOUND)
		target_link_libraries (narflib ${PNG_LIBRARIES})
	endif()
endif()

if(TEST)

	include_directories (
		"${GTEST_INCLUDE_DIRS}"
		)
	# unit test executable
	file(GLOB_RECURSE NARFLIB_TEST_SOURCE_FILES ${PROJECT_SOURCE_DIR}/test/*.cpp)

	EMBED(embed_bar_txt test/bar.txt)
	list(APPEND NARFLIB_TEST_SOURCE_FILES ${embed_bar_txt})

	if (NOT hasParent)
		set(CMAKE_C_FLAGS "${COMMON_C_FLAGS}")
		set(CMAKE_CXX_FLAGS "${COMMON_CXX_FLAGS}")

		if (NOT STATIC)
			SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
			SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
		endif()
	endif()

	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGTEST_LINKED_AS_SHARED_LIBRARY=1")

	add_executable (narflib-test ${NARFLIB_TEST_SOURCE_FILES})

	target_link_libraries (narflib-test
		narflib
		${GTEST_LIBRARIES}
		)
	if (ZLIB_FOUND)
		target_link_libraries (narflib-test ${ZLIB_LIBRARY})
	endif()
	if (WIN32)
		target_link_libraries (narflib-test ws2_32)
	endif()
endif()
