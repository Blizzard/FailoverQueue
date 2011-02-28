include_directories(. ../include/)

find_library(BOOST_THR NAMES boost_thread boost_thread-mt)
find_library(BOOST_SER NAMES boost_serialization boost_serialization-mt)
find_library(BOOST_SYS NAMES boost_system boost_system-mt)
find_library(BOOST_FS NAMES boost_filesystem boost_filesystem-mt)

add_definitions(-Wall -Wall -Wextra -g -gstabs+)

add_executable(01_basic 01_basic.cpp)
# set_target_properties(01_basic PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
TARGET_LINK_LIBRARIES(01_basic ${BOOST_SER} ${BOOST_SYS} ${BOOST_FS} ${BOOST_THR})

add_executable(02_complex 02_complex.cpp)
# set_target_properties(02_complex PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
TARGET_LINK_LIBRARIES(02_complex ${BOOST_SER} ${BOOST_SYS} ${BOOST_FS} ${BOOST_THR})

add_executable(03_uneven 03_uneven.cpp)
# set_target_properties(03_uneven PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
TARGET_LINK_LIBRARIES(03_uneven ${BOOST_SER} ${BOOST_SYS} ${BOOST_FS} ${BOOST_THR})

add_executable(04_even 04_even.cpp)
# set_target_properties(04_even PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
TARGET_LINK_LIBRARIES(04_even ${BOOST_SER} ${BOOST_SYS} ${BOOST_FS} ${BOOST_THR})

add_executable(05_order 05_order.cpp)
# set_target_properties(05_order PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
TARGET_LINK_LIBRARIES(05_order ${BOOST_SER} ${BOOST_SYS} ${BOOST_FS} ${BOOST_THR})

add_executable(06_missing 06_missing.cpp)
# set_target_properties(06_missing PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
TARGET_LINK_LIBRARIES(06_missing ${BOOST_SER} ${BOOST_SYS} ${BOOST_FS} ${BOOST_THR})

ENABLE_TESTING()

ADD_TEST(01_basic 01_basic)
ADD_TEST(02_complex 02_complex)
ADD_TEST(03_uneven 03_uneven)
ADD_TEST(04_even 04_even)
ADD_TEST(05_order 05_order)
ADD_TEST(06_missing 06_missing)

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} DEPENDS 01_basic 02_complex 03_uneven 04_even 05_order 06_missing)
