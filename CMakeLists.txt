cmake_minimum_required (VERSION 2.6)

project (DICOMReader)

include_directories ("${PROJECT_SOURCE_DIR}/DICOMParser/src")
include_directories ("${CMAKE_BINARY_DIR}/DICOMParser/src")

add_subdirectory (DICOMParser/src) 
 
add_executable(DICOMReader DICOMReader.cpp)

target_link_libraries (DICOMReader ITKDICOMParser)
