
aux_source_directory(. DIR_LIB_SRCS)
LINK_LIBRARIES(pthread)

set(CMAKE_INCLUDE_PATH "./base")

add_library (mnet ${DIR_LIB_SRCS})
target_link_libraries(mnet mbase)