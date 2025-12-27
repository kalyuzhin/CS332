cmake_minimum_required(VERSION 3.28)
project(CS332)

set(CMAKE_CXX_STANDARD 20)

set(SOURCES
    main.cpp
    provider.h
)

file(GLOB_RECURSE INDIV3 "indiv3/*.cpp" "indiv3/*.h")
list(APPEND SOURCES ${INDIV3})

message(STATUS "Sources: ${SOURCES}")

add_executable(CS332 ${SOURCES})

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

if(APPLE)
    find_package(glfw3 REQUIRED)
endif()
find_package(OpenGL REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(GLEW REQUIRED)
find_package(assimp REQUIRED)
find_package(SFML CONFIG REQUIRED COMPONENTS System Window Graphics)


target_link_libraries(CS332 
    ${OpenCV_LIBS}
    glfw
    ${WIN32_LIBRARIES}
    GLEW::GLEW
    glm::glm
    assimp::assimp
    SFML::System
    SFML::Window
    SFML::Graphics
)

target_include_directories(CS332 PRIVATE ${CMAKE_SOURCE_DIR})
target_include_directories(CS332 PRIVATE build/imgui build/imgui/backends)
target_include_directories(CS332 PRIVATE build/glfw build/glfw/include/GLFW)
target_include_directories(CS332 PRIVATE build/imfilebrowser)
if (WIN32)
    target_compile_definitions(CS332 PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(${PROJECT_NAME} opengl32 gdi32)
endif ()