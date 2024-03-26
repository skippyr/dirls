cmake_minimum_required(VERSION 3.20)
project(dirls)
add_executable(dirls "${CMAKE_SOURCE_DIR}/src/dirls.c" "${CMAKE_SOURCE_DIR}/src/dirls.h")
install(TARGETS dirls DESTINATION "${CMAKE_SOURCE_DIR}/bin")
