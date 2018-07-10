cmake_minimum_required( VERSION 3.8 )

set( CMAKE_CXX_STANDARD 11 )

project( jssy )

add_library( jssy jssy.c jssy.cpp )
target_include_directories( jssy PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )

if( BUILD_EXE )
    add_executable( jssy_exe cppmain.cpp cmain.c )
    target_link_libraries( jssy_exe jssy )
endif()

