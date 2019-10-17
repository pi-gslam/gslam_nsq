cmake_minimum_required(VERSION 2.8)
include(cmake/PICMake.cmake)

set(CMAKE_CXX_STANDARD 11)

#add_subdirectory(evpp)

INCLUDE_DIRECTORIES(./src)

pi_add_target(gslam_nsq SHARED src REQUIRED System GSLAM DEPENDENCY event)
pi_report_targets()
