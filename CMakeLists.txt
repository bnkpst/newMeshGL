cmake_minimum_required(VERSION 3.14)
project(mesh)

set(CMAKE_CXX_STANDARD 14)

add_executable(${PROJECT_NAME} main.cpp shader.h)

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES})

find_package(glm REQUIRED)
include_directories(${glm_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${glm_LIBRARIES})

find_package(GDAL REQUIRED)
include_directories(${GDAL_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${GDAL_LIBRARIES})

find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${GLEW_LIBRARIES})

include_directories(/usr/include/GLFW)
target_link_libraries(${PROJECT_NAME} /usr/lib/x86_64-linux-gnu/libglfw.so)



