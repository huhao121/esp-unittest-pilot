idf_component_register(SRCS "lamp.c"
                       INCLUDE_DIRS "include")

# 覆盖率开关：idf.py -DLAMP_COVERAGE=ON build 时只给 lamp 自己插桩。
# 平时构建（含烧板子）不开，零影响。
if(LAMP_COVERAGE)
    target_compile_options(${COMPONENT_LIB} PRIVATE --coverage -O0 -g)
    target_link_options(${COMPONENT_LIB} PUBLIC --coverage)
endif()
