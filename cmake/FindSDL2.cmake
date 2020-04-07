#==--- ripple/glow/cmake/FindSDL2.cmake -------------------------------------==#
#
#                      Copyright (c) 2020 Ripple
#
#  This file is distributed under the MIT License. See LICENSE for details.
#
#==--------------------------------------------------------------------------==#
# Locate SDL2 library
# This module defines
# SDL2_LIBRARY, the name of the library to link against
# SDL2_FOUND, if false, do not try to link to SDL2
# SDL2_INCLUDE_DIR, where to find SDL.h
#=============================================================================
# Modified by Rob Clucas, April 2020.
# This is a modified version of others which can be found online. It does not 
# find the SDL2main since it's not needed. It also _does not_ automatically
# add cocoa as a framework, since that gets done already.
#=============================================================================
# Copyright 2003-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

set(
  SDL2_SEARCH_PATHS
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local
	/usr
	/sw         # Fink
	/opt/local  # DarwinPorts
	/opt/csw    # Blastwave
	/opt
)

find_path(
  SDL2_INCLUDE_DIR SDL.h
	HINTS
	$ENV{SDL2DIR}
	PATH_SUFFIXES include/SDL2 include
	PATHS ${SDL2_SEARCH_PATHS}
)

find_library(
  SDL2_LIBRARY_TEMP
	NAMES SDL2
	HINTS
	$ENV{SDL2DIR}
	PATH_SUFFIXES lib64 lib
	PATHS ${SDL2_SEARCH_PATHS}
)

# SDL2 may require threads.
# The Apple build may not need an explicit flag because one of the
# frameworks may already provide it.
# But for non-OSX systems, I will use the CMake Threads package.
if(NOT APPLE)
	find_package(Threads)
endif(NOT APPLE)

# MinGW needs an additional library, mwindows
# It's total link flags should look like -lmingw32 -lSDL2main -lSDL2 -lmwindows
# (Actually on second look, I think it only needs one of the m* libraries.)
if(MINGW)
	set(MINGW32_LIBRARY mingw32 CACHE STRING "mwindows for MinGW")
endif(MINGW)

if(SDL2_LIBRARY_TEMP)
  # Usually, we would need "-framework Cocoa", but we have already added it.
	#  Also dont need threads for apple
	if(APPLE)
		set(SDL2_LIBRARY_TEMP ${SDL2_LIBRARY_TEMP})
	else()
		set(SDL2_LIBRARY_TEMP ${SDL2_LIBRARY_TEMP} ${CMAKE_THREAD_LIBS_INIT})
	endif(APPLE)

	# For MinGW library
	if(MINGW)
		set(SDL2_LIBRARY_TEMP ${MINGW32_LIBRARY} ${SDL2_LIBRARY_TEMP})
	endif(MINGW)

	# Set the final string here so the GUI reflects the final state.
	set(
		SDL2_LIBRARY 
		${SDL2_LIBRARY_TEMP}
		CACHE STRING "Where the SDL2 Library can be found"
	)
	# Set the temp variable to INTERNAL so it is not seen in the CMake GUI
	set(SDL2_LIBRARY_TEMP "${SDL2_LIBRARY_TEMP}" CACHE INTERNAL "")
endif(SDL2_LIBRARY_TEMP)

include(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
  SDL2 REQUIRED_VARS SDL2_LIBRARY SDL2_INCLUDE_DIR
)