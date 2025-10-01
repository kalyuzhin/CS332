cmake_minimum_required(VERSION 3.28)
project(CS332)

set(CMAKE_CXX_STANDARD 20)


add_executable(CS332 main.cpp
        Lab03/Task1/task1.cpp
        Lab03/Task1/task1.h
        provider.h
        Lab03/Task3/Task3.cpp 
        Lab03/Task3/Task3.h)

if (DEFINED ENV{OpenCV_DIR})
    set(OpenCV_DIR $ENV{OpenCV_DIR})
endif ()

find_package(OpenCV REQUIRED)
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
message(STATUS "OpenCV include dir: ${OpenCV_INCLUDE_DIRS}")



target_link_libraries(CS332 ${OpenCV_LIBS})
target_include_directories(CS332 PRIVATE ${OpenCV_INCLUDE_DIRS})
if(WIN32)
    target_compile_definitions(CS332 PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()