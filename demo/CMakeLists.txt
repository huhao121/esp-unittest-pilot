cmake_minimum_required(VERSION 3.16)

# demo 工程根在 demo/，把仓库根的 components/ 加进来才能用到 lamp
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(lamp_demo)
