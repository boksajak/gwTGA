cmake_minimum_required (VERSION 2.6) 
project (gwTGA) 

include(../cmake/macros.cmake)

set_property(GLOBAL PROPERTY USE_FOLDERS On)

MACRO(SUBDIRLIST result curdir)
  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
  SET(dirlist "")
  FOREACH(child ${children})
    IF(IS_DIRECTORY ${curdir}/${child})
        SET(dirlist ${dirlist} ${child})
    ENDIF()
  ENDFOREACH()
  SET(${result} ${dirlist})
ENDMACRO()

file(GLOB_RECURSE INCS "*.h")
file(GLOB_RECURSE CPPS "*.cpp")

add_executable(gwTGATest ${CPPS} ${INCS})

set(COPY_TARGET_DIR $<TARGET_FILE_DIR:gwTGATest>)
post_build_make_dir_link(gwTGATest ${PROJECT_SOURCE_DIR}/../test_images  ${COPY_TARGET_DIR}/test_images) 
if(MSVC) 
	post_build_make_dir_link(gwTGATest ${PROJECT_SOURCE_DIR}/../test_images  ${PROJECT_BINARY_DIR}/test_images) 
endif(MSVC) 
