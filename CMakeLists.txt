cmake_minimum_required(VERSION 3.26)

project(difftest)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 17)

include(GNUInstallDirs)

include_directories(include)
add_subdirectory(src)
