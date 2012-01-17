# Copyright (c) 2011 Prime Focus Film.
# 
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 
# Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
# Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the
# distribution. Neither the name of Prime Focus Film nor the
# names of its contributors may be used to endorse or promote
# products derived from this software without specific prior written
# permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.

# Author : Antoine P. 

project( Field3DPlugin )

cmake_minimum_required(VERSION 2.8)
set( CMAKE_VERBOSE_MAKEFILE on )

# add the .h and .cpp files 
file(GLOB SOURCES_FILES ./src/*.h ./src/*.cpp)
add_library( Field3DPlugin SHARED ${SOURCES_FILES} )


# Rpath's are so bad, we don't want them ... 
# Use your LD_LIBRARY_PATH instead
set ( CMAKE_SKIP_RPATH TRUE )


# -D_BOOL -DREQUIRE_IOSTREAM seems to be mandatory when
# using Maya's include files
#set ( CMAKE_CXX_FLAGS "-Wno-deprecated -D_BOOL -DREQUIRE_IOSTREAM -DLINUX" ) 
set ( CMAKE_CXX_FLAGS "-Wl,-soname,libField3DPlugin.so -D_BOOL -DREQUIRE_IOSTREAM -DLINUX" )


# set the build type to release by default
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif(NOT CMAKE_BUILD_TYPE)

set ( LINK_TYPE static CACHE STRING
      "Choose the type of link, options are: static or dynamic ( static is the default )."
      )

#--------------------------------------------------------------------------
#   add_custom_library ()
#
#   description:
#      This function locates and links against third party libraries  
#      ( Maya, Ilmbase, Hdf5 ). Libraries are supposed to have a root 
#      directory with an "include" and "lib" directory inside.
#
#   usage : 
#       add_custom_library ( NAME INCLUDE_FILE LIBRARIES )
#         NAME         : library name
#         INCLUDE_FILE : the name of any header file in 
#                        library's include directory
#         LIBRARIES    : list of shared library to link against 
#                        ( without prefix, nor suffix : libAA.so -> AA )
#
#   Example :
#      add_custom_library ( IlmBase OpenEXR/Iex.h   IlmThread Iex Imath Half )
#----------------------------------------------------------------------------
 

function( add_custom_library NAME INCLUDE_FILE IS_STATIC LIBRARIES )
	
	string(TOUPPER ${NAME} LIB_NAME)
	
	# look for the root path of the library
	find_path ( ${LIB_NAME}_ROOT_DIR PATHS 
		include
		DOC "${NAME} root directory" 
		NO_DEFAULT_PATH 
	)
	
	# check if the root path was found
	if( NOT ${LIB_NAME}_ROOT_DIR )
		message ( SEND_ERROR "Please set ${LIB_NAME}_ROOT_DIR to the ${NAME} root directory" )
	endif()
	
	# look up the include directory automatically
	set (${LIB_NAME}_INCLUDE_DIR ${LIB_NAME}_INCLUDE_DIR-NOTFOUND )
	find_path( ${LIB_NAME}_INCLUDE_DIR 
		NAMES ${INCLUDE_FILE}
		PATHS ${${LIB_NAME}_ROOT_DIR}/include/ 
	    DOC "Path to the ${NAME} include directory ( containing ${INCLUDE_FILE} )."
	) 
	
	# check if include dir was found
	if( NOT ${LIB_NAME}_INCLUDE_DIR )
		if( ${LIB_NAME}_ROOT_DIR )
			message ( SEND_ERROR "No ${NAME} include directory was found in ${${LIB_NAME}_ROOT_DIR} , Check your ${LIB_NAME}_ROOT_DIR again." )
		endif()
	else( NOT ${LIB_NAME}_INCLUDE_DIR )
		include_directories( ${${LIB_NAME}_INCLUDE_DIR} )
	endif()
	
	mark_as_advanced(${LIB_NAME}_INCLUDE_DIR) 	


	# adding libraries specified in LIBRARIES argument
	foreach ( LIB ${LIBRARIES} )
	
		# clean previous value
		set (${LIB}_LIBRARY ${LIB}_LIBRARY-NOTFOUND )
	
		# look for the library
		if ( ${IS_STATIC} MATCHES "static"  )
			set (LIB lib${LIB}.a)
		endif()
		
		find_library( ${LIB}_LIBRARY 
			NAMES ${LIB}
			PATHS ${${LIB_NAME}_ROOT_DIR}/lib
			DOC "${LIB} library."
		)
		
		mark_as_advanced(${LIB}_LIBRARY)
		
		# check if the library was found
		if( NOT ${LIB}_LIBRARY )
			if( ${LIB_NAME}_ROOT_DIR )
				message ( SEND_ERROR "${LIB} library directory was not found in ${${LIB_NAME}_ROOT_DIR}/lib , Check your ${LIB_NAME}_ROOT_DIR or again." )
			endif()
		else( NOT ${LIB}_LIBRARY )
				target_link_libraries(Field3DPlugin ${${LIB}_LIBRARY} )
		endif()
	
	endforeach()
	
endfunction(add_custom_library)


#----------------------------------------------------------------------------


# add libraries : WARNING add Field3D first !!
# don't forget the quotes when passing a list
add_custom_library ( Field3D Field3D/Field.h ${LINK_TYPE}  "Field3D"    )
add_custom_library ( Maya    maya/MFn.h      FALSE "OpenMaya;OpenMayaFX;OpenMayaUI;OpenMayaAnim" )
add_custom_library ( HDF5    hdf5.h          ${LINK_TYPE}  "hdf5"      ) 
add_custom_library ( IlmBase OpenEXR/Iex.h   ${LINK_TYPE}  "IlmThread;Iex;Imath;Half" ) 


# Hack:
# We must add ilmbase/OpenEXR in include directories as well
# because ilmbase reference OpenEXR header as 
#       #include "Iex.h" 
# instead of 
#       #include "OpenEXR/Iex.h" 

if( ILMBASE_ROOT_DIR )
	include_directories( ${ILMBASE_ROOT_DIR}/include/OpenEXR )
endif()

		



