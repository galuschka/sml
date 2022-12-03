cmake_minimum_required(VERSION 3.0.0)
project(sml VERSION 0.1.0)

option(DZGFIX "use smlfixup_dzg to fixup smart meter error" OFF)

include(CTest)
enable_testing()

if(DZGFIX)
    set(fixup smlfixup_dzg.cpp)
else()
    set(fixup smlfixup_null.cpp)
endif()

add_executable(smldump  main.cpp crc16.cpp sml.cpp ${fixup} smldump.cpp smldumpinst.cpp)
add_executable(obisdump main.cpp crc16.cpp sml.cpp ${fixup} smldump.cpp obis.cpp obisid.cpp obisunit.cpp obisdump.cpp)
add_executable(mqttdump main.cpp crc16.cpp sml.cpp ${fixup} obis.cpp filter.cpp mqttdump.cpp)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
