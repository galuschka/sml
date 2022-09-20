cmake_minimum_required(VERSION 3.0.0)
project(sml VERSION 0.1.0)

include(CTest)
enable_testing()

add_executable(smldump  main.cpp crc16.cpp sml.cpp smldump.cpp smldumpinst.cpp)
add_executable(obisdump main.cpp crc16.cpp sml.cpp smldump.cpp obis.cpp obisid.cpp obisunit.cpp obisdump.cpp)
add_executable(mqttdump main.cpp crc16.cpp sml.cpp smldump.cpp obis.cpp filter.cpp mqttdump.cpp)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
