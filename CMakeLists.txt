cmake_minimum_required( VERSION 3.10 )
project( lab1 )

set( FREEGLUT_BUILD_DEMOS FALSE )
add_subdirectory( libs/freeglut-3.4.0 )
find_package(OpenGL)
include_directories( libs/freeglut-3.4.0/include include )
link_libraries( freeglut_static ${OPENGL_LIBRARIES} )

add_executable( solar_system src/solar_system.cpp )
add_executable( solar_system_lessvmath src/solar_system_lessvmath.cpp )
add_executable( test src/test.cpp )
add_executable( exercise src/exercise.cpp )

