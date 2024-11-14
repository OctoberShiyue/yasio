set(target_name echo_server)

set (ECHO_SERVER_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (ECHO_SERVER_INC_DIR 
    ${ECHO_SERVER_SRC_DIR}/lib
)


file(GLOB ECHO_SERVER_SRC
    "${ECHO_SERVER_SRC_DIR}/*.cpp"
    "${ECHO_SERVER_SRC_DIR}/*.h"

    "${ECHO_SERVER_SRC_DIR}/lib/base64/*.cpp"
    "${ECHO_SERVER_SRC_DIR}/lib/base64/*.h"

    "${ECHO_SERVER_SRC_DIR}/lib/json/*.cpp"

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
    "Crypt32.lib"
    "${ECHO_SERVER_SRC_DIR}/lib/lua54/lua54.lib"

)

yasio_config_app_depends(${target_name})
