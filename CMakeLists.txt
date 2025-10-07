cmake_minimum_required(VERSION 3.28)
project(CS332)

set(CMAKE_CXX_STANDARD 20)


add_executable(CS332 main.cpp
        Lab03/Task1/task1.cpp
        Lab03/Task1/task1.h
        provider.h
        lab04/lab.cpp
        lab04/lab.h
        #        Lab03/Task3/Task3.cpp
        #        Lab03/Task3/Task3.h
        #        Lab03/Task2/Task2.cpp)
)

if (DEFINED ENV{OpenCV_DIR})
    set(OpenCV_DIR $ENV{OpenCV_DIR})
endif ()

target_sources(${PROJECT_NAME} PRIVATE
        build/imgui/imgui.cpp
        build/imgui/imgui_demo.cpp
        build/imgui/imgui_draw.cpp
        build/imgui/imgui_tables.cpp
        build/imgui/imgui_widgets.cpp
        build/imgui/backends/imgui_impl_glfw.cpp
        build/imgui/backends/imgui_impl_metal.h
)

find_package(OpenCV REQUIRED)
#find_package(imgui REQUIRED)
#find_package(glfw3 REQUIRED)
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
message(STATUS "OpenCV include dir: ${OpenCV_INCLUDE_DIRS}")


target_link_libraries(CS332 ${OpenCV_LIBS})
target_include_directories(CS332 PRIVATE ${OpenCV_INCLUDE_DIRS})
target_include_directories(CS332 PRIVATE build/imgui build/imgui/backends)
if (WIN32)
    target_compile_definitions(CS332 PRIVATE _CRT_SECURE_NO_WARNINGS)
endif ()