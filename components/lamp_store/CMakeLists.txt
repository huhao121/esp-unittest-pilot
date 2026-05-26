idf_component_register(SRCS "lamp_store.c"
                       INCLUDE_DIRS "include"
                       REQUIRES lamp)

# 覆盖率开关：idf.py -DLAMP_STORE_COVERAGE=ON build 时只给 lamp_store 自己插桩。
# 平时构建（含烧板子）不开，零影响。
if(LAMP_STORE_COVERAGE)
    target_compile_options(${COMPONENT_LIB} PRIVATE --coverage -O0 -g)
    target_link_options(${COMPONENT_LIB} PUBLIC --coverage)
endif()
