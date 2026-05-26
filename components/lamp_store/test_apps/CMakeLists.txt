cmake_minimum_required(VERSION 3.16)

# 把上层 components/ 目录加进来，好让本测试工程找到 lamp_store / lamp 组件
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../..")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(lamp_store_test)
