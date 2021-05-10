cmake_minimum_required(VERSION 3.10.2)

project("camera_v4l2")

set(MODULE_NAME camera)
set(CMAKE_BUILD_TYPE RELEASE)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -s")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -s")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")

option(LOG_SWITCH "option for print log" ON)
if(LOG_SWITCH)
    add_definitions(-DLOG_SWITCH)
endif(LOG_SWITCH)

include_directories(source)
aux_source_directory(source DIR_SRC)
add_library(${MODULE_NAME} SHARED ${DIR_SRC})

find_library(log-lib log)

target_link_libraries(${MODULE_NAME} ${log-lib} mediandk android jnigraphics)