cmake_minimum_required(VERSION 3.28)
project(CS332)

set(CMAKE_CXX_STANDARD 20)

set(SOURCES
    main.cpp
    provider.h
        lab06/lab.h
        lab07/task2.cpp
        lab07/task2.h
        greenTriangleMark/triangle.h
        lab11/lab11.h
        lab12/lab12.h
)

file(GLOB_RECURSE LAB04_FILES "Lab04/*.cpp" "Lab04/*.h")
list(APPEND SOURCES ${LAB04_FILES})
file(GLOB_RECURSE LAB05_FILES "Lab05/*.cpp" "Lab05/*.h")
list(APPEND SOURCES ${LAB05_FILES})
message(STATUS "Sources: ${SOURCES}")

add_executable(CS332 ${SOURCES})

if (DEFINED ENV{OpenCV_DIR})
    set(OpenCV_DIR $ENV{OpenCV_DIR})
endif ()

target_sources(${PROJECT_NAME} PRIVATE
        build/imgui/imgui.cpp
        build/imgui/imgui_draw.cpp
        build/imgui/imgui_tables.cpp
        build/imgui/imgui_widgets.cpp
        build/imgui/backends/imgui_impl_glfw.cpp
        build/imgui/backends/imgui_impl_metal.h
        build/imgui/backends/imgui_impl_opengl3.cpp
)

if(WIN32)
    add_subdirectory(build/glfw)
    add_definitions(-D_GLFW_WIN32)
    set(WIN32_LIBRARIES
        opengl32
        gdi32
    )
endif()

find_package(OpenCV REQUIRED)
#find_package(imgui REQUIRED)
if(APPLE)
    find_package(glfw3 REQUIRED)
endif()
find_package(OpenGL REQUIRED)
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
message(STATUS "OpenCV include dir: ${OpenCV_INCLUDE_DIRS}")


target_link_libraries(CS332 
    ${OpenCV_LIBS}
    glfw
    ${WIN32_LIBRARIES}
)

if(WIN32)
    target_link_directories(CS332 PRIVATE ${CMAKE_SOURCE_DIR}/build/TriangleDependences/lib)
    target_link_directories(CS332 PRIVATE ${CMAKE_SOURCE_DIR}/build/TriangleDependences/bin)
    target_link_libraries(CS332
        glew32
        sfml-graphics-d
        sfml-window-d
        sfml-system-d
)
endif()

target_include_directories(CS332 PRIVATE ${CMAKE_SOURCE_DIR})
target_include_directories(CS332 PRIVATE ${OpenCV_INCLUDE_DIRS})
target_include_directories(CS332 PRIVATE build/imgui build/imgui/backends)
target_include_directories(CS332 PRIVATE build/glfw build/glfw/include/GLFW)
target_include_directories(CS332 PRIVATE build/imfilebrowser)
target_include_directories(CS332 PRIVATE build/TriangleDependences build/TriangleDependences/include)

if (WIN32)
    target_compile_definitions(CS332 PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(${PROJECT_NAME} opengl32 gdi32)
endif ()