cmake_minimum_required(VERSION 2.8)
project( image_seg )
file( GLOB SOURCES *.cpp )
find_package( OpenCV REQUIRED )
add_executable( image_seg ${SOURCES} )
target_link_libraries( image_seg ${OpenCV_LIBS} )
