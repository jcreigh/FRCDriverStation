cmake_minimum_required (VERSION 2.6)
project (SimpleDS)

option(STATIC "Use static linking when building executables" OFF)
option(PIE "Create position-independent executables" ON)

if (PIE AND STATIC)
	# PIE + STATIC will result in TEXTRELs unless all libraries have been
	# built for PIC/PIE, so don't allow it
	set(PIE OFF)
endif()

set (VERSION_MAJOR 0)
set (VERSION_MINOR 1)
set (VERSION_RELEASE "")

if (NOT DEFINED PATCH_COMMAND)
  set(PATCH_COMMAND "patch")
endif()

if (NOT DEFINED GIT_COMMAND)
  set(GIT_COMMAND "git")
endif()

execute_process(COMMAND ${GIT_COMMAND} describe --tags --always --dirty OUTPUT_VARIABLE VERSION_REV OUTPUT_STRIP_TRAILING_WHITESPACE)

# generate list of authors separated by C newlines
execute_process(COMMAND ${GIT_COMMAND} shortlog -sn HEAD OUTPUT_VARIABLE VERSION_AUTHORS OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REGEX REPLACE "[ ]+[0-9]+\t" "" VERSION_AUTHORS ${VERSION_AUTHORS})
string(REPLACE "\n" "\\n" VERSION_AUTHORS ${VERSION_AUTHORS})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/../cmake/Modules/")
set(SIMPLEDS_DATA_PATH "${CMAKE_SOURCE_DIR}/../data/")

find_package (SDL2 REQUIRED)
find_package (SDL2_ttf REQUIRED)
find_package (Threads REQUIRED)
find_package (CURL REQUIRED)

if (CMAKE_CXX_COMPILER_ID STREQUAL Clang)
	set(CMAKE_COMPILER_IS_CLANG 1)
endif ()

if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANG)
	SET(CMAKE_C_FLAGS
		"${CMAKE_C_FLAGS} -g -Wpointer-arith -Wtype-limits -Wwrite-strings -Wuninitialized -Werror=redundant-decls -Wsign-compare -Wconversion -g -fno-ident")

	# don't export symbols (especially for PIC builds, but it doesn't hurt elsewhere)
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")

	if (STATIC)
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static -static-libgcc -static-libstdc++")
	endif()

	# position-independent executable
	if (PIE)
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIE")
		SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie -Wl,-z,relro,-z,now")
	endif()

	SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed")

	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++11")
endif ()

if (MSVC)
	# ask MSVC to define M_PI and related constants
	add_definitions( -D_USE_MATH_DEFINES )
endif ()

set(TEST OFF)
add_subdirectory(${CMAKE_SOURCE_DIR}/narflib)

include_directories (
	"${SDL2_INCLUDE_DIR}"
	"${SDL2_TTF_INCLUDE_DIR}"
	"${CURL_INCLUDE_DIRS}"
	"${NARFLIB_INCLUDE_DIRS}"
	"${PROJECT_BINARY_DIR}"
	"${PROJECT_SOURCE_DIR}"
	)

configure_file (
	"${PROJECT_SOURCE_DIR}/version.h.in"
	"${PROJECT_BINARY_DIR}/version.h"
	)

# source files used by both client and server
EMBED(embed_DroidSansMono_ttf ../data/DroidSansMono.ttf)

set (SIMPLEDS_SOURCE_FILES
	main.cpp
	net.cpp
	GUI.cpp
	DS.cpp
	RoboRIO.cpp
	enums.cpp
	rioversions.cpp
	joystick.cpp
	screen.cpp
	config.cpp
	${embed_DroidSansMono_ttf}
	)

add_executable (SimpleDS
	${SIMPLEDS_SOURCE_FILES}
	)

target_link_libraries (SimpleDS
	narflib
	${SDL2_LIBRARY}
	${SDL2_TTF_LIBRARY}
	${CURL_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
	)

if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
	set_target_properties (SimpleDS
		PROPERTIES LINK_FLAGS "-Wl,-Map=SimpleDS.map"
		)
endif ()

if (NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE Release CACHE STRING
		"Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
		FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)
