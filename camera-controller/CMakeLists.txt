cmake_minimum_required(VERSION 3.18)

project(hawk-controller VERSION 0.1.0)

set (CMAKE_CXX_STANDARD 20)

file(GLOB SRCS
     "*.cpp"
)

file(GLOB HDRS
     "*.h"
)

ADD_LIBRARY(${PROJECT_NAME} STATIC ${SRCS} ${HDRS})

find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})

target_include_directories(${PROJECT_NAME} PUBLIC /usr/local/include/libcamera)
target_include_directories(${PROJECT_NAME} PUBLIC /usr/include/libcamera)

target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME}
    libcamera.so
    libcamera-base.so
)