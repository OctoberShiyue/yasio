set(target_name echo_server)

set (ECHO_SERVER_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (ECHO_SERVER_INC_DIR 
    ${ECHO_SERVER_SRC_DIR}/../../
    ${ECHO_SERVER_SRC_DIR}/lib
)

# set (ECHO_SERVER_SRC 
# ${ECHO_SERVER_SRC_DIR}/main.cpp

# ${ECHO_SERVER_SRC_DIR}/lib/rsa/bigint.cpp
# ${ECHO_SERVER_SRC_DIR}/lib/rsa/hex.cpp
# ${ECHO_SERVER_SRC_DIR}/lib/rsa/rsa.cpp
# ${ECHO_SERVER_SRC_DIR}/lib/rsa/sha_1.cpp
# ${ECHO_SERVER_SRC_DIR}/lib/rsa/mini-gmp.c
# ${ECHO_SERVER_SRC_DIR}/lib/rsa/bigint.h
# ${ECHO_SERVER_SRC_DIR}/lib/rsa/hex.h
# ${ECHO_SERVER_SRC_DIR}/lib/rsa/mini-gmp.h
# ${ECHO_SERVER_SRC_DIR}/lib/rsa/minigmp_global.h
# ${ECHO_SERVER_SRC_DIR}/lib/rsa/rsa.h
# ${ECHO_SERVER_SRC_DIR}/lib/rsa/sha_1.h

# ${ECHO_SERVER_SRC_DIR}/lib/aes/AES.h
# ${ECHO_SERVER_SRC_DIR}/lib/aes/AES.cpp

# )

file(GLOB ECHO_SERVER_SRC
    "${ECHO_SERVER_SRC_DIR}/*.cpp"
    "${ECHO_SERVER_SRC_DIR}/*.h"
    "${ECHO_SERVER_SRC_DIR}/lib/rsa/*.cpp"
    "${ECHO_SERVER_SRC_DIR}/lib/rsa/*.c"
    "${ECHO_SERVER_SRC_DIR}/lib/rsa/*.h"

    "${ECHO_SERVER_SRC_DIR}/lib/mysql/include/*.cpp"
    "${ECHO_SERVER_SRC_DIR}/lib/mysql/include/*.h"
    "${ECHO_SERVER_SRC_DIR}/lib/mysql/include/mysql/*.cpp"
    "${ECHO_SERVER_SRC_DIR}/lib/mysql/include/mysql/*.h"
    "${ECHO_SERVER_SRC_DIR}/lib/mysql/include/mysql/psi/*.cpp"
    "${ECHO_SERVER_SRC_DIR}/lib/mysql/include/mysql/psi/*.h"
)

include_directories ("${ECHO_SERVER_SRC_DIR}")
include_directories ("${ECHO_SERVER_INC_DIR}")

add_executable (${target_name} ${ECHO_SERVER_SRC}) 

target_link_libraries(${target_name}
    "${ECHO_SERVER_SRC_DIR}/lib/mysql/libmysql.lib"
    "${ECHO_SERVER_SRC_DIR}/lib/mysql/mysqlclient.lib"
)

yasio_config_app_depends(${target_name})
