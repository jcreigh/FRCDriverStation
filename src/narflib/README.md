NarfLib
=======

Generic utility code that can be used outside of NarfBlock.

See the [NarfBlock source](https://github.com/narfblock/narfblock/tree/master/src/lib) for the most up to date version.

# Building

## Prerequisites

- CMake 2.6 or newer
- zlib development libraries (optional, used for compressing files to embed)
- GoogleTest (gtest) development libraries (optional, used for unit tests)

## Build Commands

First get a copy of the source:

	git clone https://github.com/narfblock/narflib.git

Make a new directory to build everything in

	mkdir build
	cd build

Then generate a Makefile using CMake and perform the build:

	cmake .. && make

Or on Windows with MinGW in an MSYS shell:

	cmake -G "MSYS Makefiles" .. && make

