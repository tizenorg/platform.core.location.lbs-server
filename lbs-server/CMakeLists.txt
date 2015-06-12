CMAKE_MINIMUM_REQUIRED(VERSION 2.0)
PROJECT(lbs-server C CXX)

SET(SERVER_SRCS_DIR "src")
SET(server_pkgs_LDFLAGS "${server_pkgs_LDFLAGS} -ldl")
SET(SERVER_EXTRA_CFLAGS "${SERVER_EXTRA_CFLAGS} -D_GNU_SOURCE")

SET(SERVER_SRCS
	${SERVER_SRCS_DIR}/lbs_server.c
	${SERVER_SRCS_DIR}/server.c
	${SERVER_SRCS_DIR}/data_connection.c
	${SERVER_SRCS_DIR}/nmea_logger.c
	${SERVER_SRCS_DIR}/gps_plugin_module.c
	${SERVER_SRCS_DIR}/last_position.c
	${SERVER_SRCS_DIR}/setting.c
	${SERVER_SRCS_DIR}/dump_log.c
	${SERVER_SRCS_DIR}/nps_plugin_module.c
)

INCLUDE_DIRECTORIES(
	src	
	include
)

CONFIGURE_FILE(org.tizen.lbs.Providers.LbsServer.service.in org.tizen.lbs.Providers.LbsServer.service @ONLY)
INSTALL(FILES org.tizen.lbs.Providers.LbsServer.service DESTINATION /usr/share/dbus-1/system-services)
#INSTALL(FILES lbs-server.provider DESTINATION /usr/share/lbs)
INSTALL(FILES script/lbs-server DESTINATION /etc/rc.d/init.d)

CONFIGURE_FILE(lbs-server-plugin.pc.in lbs-server-plugin.pc @ONLY)
INSTALL(FILES lbs-server-plugin.pc DESTINATION ${LIB_DIR}/pkgconfig)

ADD_EXECUTABLE(${PROJECT_NAME} ${SERVER_SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${server_pkgs_LDFLAGS} -lm)
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS ${SERVER_EXTRA_CFLAGS})
SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed -pie")

INSTALL(DIRECTORY include/ DESTINATION ${INCLUDE_DIR}/lbs-server-plugin FILES_MATCHING PATTERN "*.h")

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${BIN_DIR})