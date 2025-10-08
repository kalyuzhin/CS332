cmake_minimum_required(VERSION 3.28)
project(CS332)

set(CMAKE_CXX_STANDARD 20)

set(SOURCES
    main.cpp
    provider.h
)

file(GLOB_RECURSE LAB04_FILES "Lab04/*.cpp" "Lab04/*.h")
file(GLOB LAB04_UTILS "Lab04/utils/*.cpp" "Lab04/utils/*.h")
list(APPEND SOURCES ${LAB04_UTILS})
list(APPEND SOURCES ${LAB04_FILES})
message(STATUS "Sources: ${SOURCES}")

add_executable(CS332 ${SOURCES})

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