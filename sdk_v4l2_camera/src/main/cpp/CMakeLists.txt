cmake_minimum_required(VERSION 3.10.2)
project("camera_v4l2")

# set flag
set(CMAKE_BUILD_TYPE RELEASE)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -s")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -s")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")

# set common name
set(MODULE_MAIN camera)
set(DIR_YUV     libyuv)
set(DIR_CAMERA  libcamera)
set(DIR_JPEG    libjpeg-turbo)

# log switch
option(LOG_SWITCH "option for print log" ON)
if(LOG_SWITCH)
    add_definitions(-DLOG_SWITCH)
endif(LOG_SWITCH)

# module yuv
add_subdirectory(${DIR_YUV})
# module jpeg
add_subdirectory(${DIR_JPEG})

# moudle main
include_directories(${DIR_CAMERA} ${DIR_JPEG} ${DIR_YUV}/include)
aux_source_directory(${DIR_CAMERA} DIR_SRC)
add_library(${MODULE_MAIN} SHARED ${DIR_SRC})

# find system support library
find_library(log-lib log)

# target link libraries turbojpeg
target_link_libraries(${MODULE_MAIN} ${log-lib} turbojpeg yuv mediandk android #[[jnigraphics]])