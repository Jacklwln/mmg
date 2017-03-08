## =============================================================================
##  This file is part of the mmg software package for the tetrahedral
##  mesh modification.
##**  Copyright (c) Bx INP/Inria/UBordeaux/UPMC, 2004- .
##
##  mmg is free software: you can redistribute it and/or modify it
##  under the terms of the GNU Lesser General Public License as published
##  by the Free Software Foundation, either version 3 of the License, or
##  (at your option) any later version.
##
##  mmg is distributed in the hope that it will be useful, but WITHOUT
##  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
##  FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
##  License for more details.
##
##  You should have received a copy of the GNU Lesser General Public
##  License and of the GNU General Public License along with mmg (in
##  files COPYING.LESSER and COPYING). If not, see
##  <http://www.gnu.org/licenses/>. Please read their terms carefully and
##  use this copy of the mmg distribution only if you accept them.
## =============================================================================

## =============================================================================
##
## Compilation of mmgs executable, libraries and tests
##
## =============================================================================

SET(MMGS_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src/mmgs)
SET(MMGS_BINARY_DIR ${CMAKE_BINARY_DIR}/src/mmgs)

FILE(MAKE_DIRECTORY ${MMGS_BINARY_DIR})

############################################################################
#####
#####         Fortran header: libmmgsf.h
#####
############################################################################

GENERATE_FORTRAN_HEADER (
  ${MMGS_SOURCE_DIR} libmmgs.h ${MMGS_BINARY_DIR} libmmgsf.h mmgs
  )

###############################################################################
#####
#####         Sources and libraries
#####
###############################################################################

# Header files
INCLUDE_DIRECTORIES(${MMGS_SOURCE_DIR})
INCLUDE_DIRECTORIES(${COMMON_BINARY_DIR})

# Source files
FILE(
  GLOB
  sourcemmgs_files
  ${MMGS_SOURCE_DIR}/*.c   ${MMGS_SOURCE_DIR}/*.h
  ${COMMON_SOURCE_DIR}/*.c ${COMMON_SOURCE_DIR}/*.h
  ${COMMON_BINARY_DIR}/mmgcommon.h
  )
LIST(REMOVE_ITEM sourcemmgs_files
  ${MMGS_SOURCE_DIR}/mmgs.c
  ${MMGS_BINARY_DIR}/lib${PROJECT_NAME}sf.c
  ${CMAKE_SOURCE_DIR}/src/mmg/libmmg.h
  ${CMAKE_SOURCE_DIR}/src/mmg/libmmgf.h
  ${REMOVE_FILE})
FILE(
  GLOB
  mainmmgs_file
  ${MMGS_SOURCE_DIR}/mmgs.c
  )
FILE(
  GLOB
  libmmgs_file
  ${MMGS_SOURCE_DIR}/lib${PROJECT_NAME}sf.c
  )

############################################################################
#####
#####         Compile mmgs libraries
#####
############################################################################

IF ( LIBMMGS_STATIC )
  ADD_LIBRARY(lib${PROJECT_NAME}s_a  STATIC
    ${sourcemmgs_files} ${libmmgs_file} )
  SET_TARGET_PROPERTIES(lib${PROJECT_NAME}s_a PROPERTIES OUTPUT_NAME
    ${PROJECT_NAME}s)
  TARGET_LINK_LIBRARIES(lib${PROJECT_NAME}s_a ${LIBRARIES})
  INSTALL(TARGETS lib${PROJECT_NAME}s_a
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib)
ENDIF()

# Compile shared library
IF ( LIBMMGS_SHARED )
  ADD_LIBRARY(lib${PROJECT_NAME}s_so SHARED
    ${sourcemmgs_files} ${libmmgs_file})
  SET_TARGET_PROPERTIES(lib${PROJECT_NAME}s_so PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}s)
  SET_TARGET_PROPERTIES(lib${PROJECT_NAME}s_so PROPERTIES
    VERSION ${CMAKE_RELEASE_VERSION} SOVERSION 5)
  TARGET_LINK_LIBRARIES(lib${PROJECT_NAME}s_so ${LIBRARIES})
  INSTALL(TARGETS lib${PROJECT_NAME}s_so
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib)
ENDIF()

IF ( LIBMMGS_STATIC OR LIBMMGS_SHARED )
  # mmgs header files needed for library
  SET( mmgs_headers
    ${MMGS_SOURCE_DIR}/libmmgs.h
    ${MMGS_BINARY_DIR}/libmmgsf.h
    ${COMMON_SOURCE_DIR}/libmmgtypes.h
    ${COMMON_BINARY_DIR}/libmmgtypesf.h
    )
  SET(MMGS_INCLUDE ${CMAKE_SOURCE_DIR}/include/mmg/mmgs )
  SET( mmgs_includes
    ${MMGS_INCLUDE}/libmmgs.h
    ${MMGS_INCLUDE}/libmmgsf.h
    ${MMGS_INCLUDE}/libmmgtypes.h
    ${MMGS_INCLUDE}/libmmgtypesf.h
    )
  # Install header files in /usr/local or equivalent
  INSTALL(FILES ${mmgs_headers} DESTINATION include/mmg/mmgs)

  COPY_FORTRAN_HEADER (
    ${COMMON_BINARY_DIR} libmmgtypesf.h ${MMGS_INCLUDE} libmmgtypesf.h
    mmg_fortran_header copys_libmmgtypesf )

  COPY_FORTRAN_HEADER (
    ${MMGS_BINARY_DIR} libmmgsf.h ${MMGS_INCLUDE} libmmgsf.h
    mmgs_fortran_header copy_libmmgsf
    )

  # Copy header files in project directory at configuration step
  # (generated file don't exists yet or are outdated)
  FILE(INSTALL  ${mmgs_headers} DESTINATION ${MMGS_INCLUDE}
    PATTERN "libmmg*f.h"  EXCLUDE)

  ADD_CUSTOM_TARGET(copy_s_headers ALL
    DEPENDS
    copy_libmmgsf copys_libmmgtypesf
    ${MMGS_INCLUDE}/libmmgs.h
    ${MMGS_INCLUDE}/libmmgtypes.h )


ENDIF()

############################################################################
#####
#####         Compile program to test library
#####
############################################################################

IF ( TEST_LIBMMGS )
  INCLUDE(cmake/testing/libmmgs_tests.cmake)
ENDIF()

###############################################################################
#####
#####         Compile MMGS executable
#####
###############################################################################

ADD_EXECUTABLE(${PROJECT_NAME}s
  ${sourcemmgs_files} ${mainmmgs_file})

IF ( WIN32 AND NOT MINGW AND USE_SCOTCH )
  my_add_link_flags(${PROJECT_NAME}s "/SAFESEH:NO")
ENDIF ( )

TARGET_LINK_LIBRARIES(${PROJECT_NAME}s ${LIBRARIES})
INSTALL(TARGETS ${PROJECT_NAME}s RUNTIME DESTINATION bin)

ADD_TARGET_POSTFIX(${PROJECT_NAME}s)

###############################################################################
#####
#####         Continuous integration
#####
###############################################################################

IF ( BUILD_TESTING )
  ##-------------------------------------------------------------------##
  ##------- Set the continuous integration options --------------------##
  ##-------------------------------------------------------------------##
  SET(MMGS_CI_TESTS ${CI_DIR}/mmgs )
  SET(MMG_CI_TESTS ${CI_DIR}/mmg )

  ##-------------------------------------------------------------------##
  ##--------------------------- Add tests and configure it ------------##
  ##-------------------------------------------------------------------##
  # Add runtime that we want to test for mmgs
  IF( MMGS_CI )
    IF(${CMAKE_BUILD_TYPE} MATCHES "Debug")
      FILE(TO_NATIVE_PATH ${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}s_debug  EXECUT_MMGS)
      #SET(EXECUT_MMGS ${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}s_debug)
      SET(BUILDNAME ${BUILDNAME}_debug CACHE STRING "build name variable")
    ELSEIF(${CMAKE_BUILD_TYPE} MATCHES "Release")
      SET(EXECUT_MMGS ${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}s_O3)
      SET(BUILDNAME ${BUILDNAME}_O3 CACHE STRING "build name variable")
    ELSEIF(${CMAKE_BUILD_TYPE} MATCHES "RelWithDebInfo")
      SET(EXECUT_MMGS ${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}s_O3d)
      SET(BUILDNAME ${BUILDNAME}_O3d CACHE STRING "build name variable")
    ELSEIF(${CMAKE_BUILD_TYPE} MATCHES "MinSizeRel")
      SET(EXECUT_MMGS ${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}s_Os)
      SET(BUILDNAME ${BUILDNAME}_Os CACHE STRING "build name variable")
    ELSE()
      SET(EXECUT_MMGS ${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}s)
      SET(BUILDNAME ${BUILDNAME} CACHE STRING "build name variable")
    ENDIF()

    SET ( LISTEXEC_MMGS ${EXECUT_MMGS} )

    IF ( TEST_LIBMMGS )
      SET(LIBMMGS_EXEC0_a ${EXECUTABLE_OUTPUT_PATH}/libmmgs_example0_a)
      SET(LIBMMGS_EXEC0_b ${EXECUTABLE_OUTPUT_PATH}/libmmgs_example0_b)
      SET(LIBMMGS_EXEC1   ${EXECUTABLE_OUTPUT_PATH}/libmmgs_example1)
      SET(LIBMMGS_EXEC2   ${EXECUTABLE_OUTPUT_PATH}/libmmgs_example2)

      ADD_TEST(NAME libmmgs_example0_a   COMMAND ${LIBMMGS_EXEC0_a})
      ADD_TEST(NAME libmmgs_example0_b   COMMAND ${LIBMMGS_EXEC0_b})
      ADD_TEST(NAME libmmgs_example1   COMMAND ${LIBMMGS_EXEC1})
      ADD_TEST(NAME libmmgs_example2   COMMAND ${LIBMMGS_EXEC2})

      IF ( CMAKE_Fortran_COMPILER)
        SET(LIBMMGS_EXECFORTRAN_a ${EXECUTABLE_OUTPUT_PATH}/libmmgs_fortran_a)
        SET(LIBMMGS_EXECFORTRAN_b ${EXECUTABLE_OUTPUT_PATH}/libmmgs_fortran_b)
        ADD_TEST(NAME libmmgs_fortran_a   COMMAND ${LIBMMGS_EXECFORTRAN_a})
        ADD_TEST(NAME libmmgs_fortran_b   COMMAND ${LIBMMGS_EXECFORTRAN_b})
      ENDIF()

    ENDIF()
    # Add mmgs tests
    INCLUDE( ${CMAKE_SOURCE_DIR}/cmake/testing/mmgs_tests.cmake )

    IF ( RUN_AGAIN )
      INCLUDE( ${CMAKE_SOURCE_DIR}/cmake/testing/mmgs_rerun_tests.cmake )
    ENDIF()


  ENDIF ( MMGS_CI )

ENDIF ( BUILD_TESTING )
