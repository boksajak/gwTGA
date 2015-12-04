cmake_minimum_required (VERSION 2.6) 
project (gwTGA) 

include(../cmake/macros.cmake)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Create gwTGA library "object" and static and shared library built from this object
add_library (gwTGAObject OBJECT gwTGA.cpp gwTGA.h)

add_library (gwTGALib STATIC $<TARGET_OBJECTS:gwTGAObject>)
add_library (gwTGA    SHARED $<TARGET_OBJECTS:gwTGAObject>)

# Create testing utility executable
add_executable(gwTGATest Test.cpp gwTGA.h)

# Link core library to utility application
target_link_libraries (gwTGATest gwTGALib)

# Create symbolic links to test images folder
set(COPY_TARGET_DIR $<TARGET_FILE_DIR:gwTGATest>)
post_build_make_dir_link(gwTGATest ${PROJECT_SOURCE_DIR}/../test_images  ${COPY_TARGET_DIR}/test_images) 
if(MSVC) 
	post_build_make_dir_link(gwTGATest ${PROJECT_SOURCE_DIR}/../test_images  ${PROJECT_BINARY_DIR}/test_images) 
endif(MSVC) 
                 
# Install shared, static library and CLI utility
install(TARGETS gwTGA
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

install(TARGETS gwTGALib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib/static
        ARCHIVE DESTINATION lib/static)
        
# Install header file
install(FILES gwTGA.h DESTINATION include)

