CMAKE_MINIMUM_REQUIRED(VERSION 3.13)
CMAKE_POLICY(SET CMP0048 NEW)
PROJECT(chatnet VERSION 2 DESCRIPTION "real-time client to chatnet-server" HOMEPAGE_URL "https://github.com/midnqp/chatnet-cli" LANGUAGES C)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/deps)


IF(CMAKE_BUILD_TYPE STREQUAL "Release")
MESSAGE("building release binary")
ELSE()
MESSAGE("building debug binary")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror=implicit-function-declaration -O0 -g -Wall -Wextra -fsanitize=address")
SET(BUILD_SHARED_LIBS ON)
ENDIF()

IF(DEFINED BUILD_SHARED_LIBS)
MESSAGE("building shared libraries")
SET(buildopts "")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
ADD_LIBRARY(util OBJECT ./util.c) 
ADD_LIBRARY(string OBJECT ./str.c) 
ADD_LIBRARY(sioclient OBJECT ./sio-client.c) 
ADD_LIBRARY(db OBJECT ./ipc.c)
ADD_LIBRARY(markdown OBJECT ./markdown.c)
ADD_LIBRARY(microphone OBJECT ./microphone.c)

ADD_LIBRARY(linenoise OBJECT ./deps/linenoise/linenoise.c)
ADD_LIBRARY(sc OBJECT ./deps/sc/sc_log.c)

ADD_LIBRARY(libraries SHARED $<TARGET_OBJECTS:util>
						   $<TARGET_OBJECTS:string>
						   $<TARGET_OBJECTS:sioclient>
						   $<TARGET_OBJECTS:db>
						   $<TARGET_OBJECTS:markdown>
						   $<TARGET_OBJECTS:microphone>)
ADD_LIBRARY(deps SHARED  $<TARGET_OBJECTS:linenoise> $<TARGET_OBJECTS:sc>) 
ELSE()
MESSAGE("building static libraries")
SET(buildopts "-static")
ADD_LIBRARY(libraries STATIC ./util.c ./str.c ./sio-client.c ./ipc.c ./markdown.c ./microphone.c)
ADD_LIBRARY(deps STATIC ./deps/linenoise/linenoise.c ./deps/sc/sc_log.c)
ENDIF()

SET(downloaded-deps json-c uuid gc)
ADD_EXECUTABLE(chatnet client.c)
TARGET_LINK_LIBRARIES(chatnet ${buildopts} libraries deps ${downloaded-deps} ${CMAKE_CURRENT_SOURCE_DIR}/lib/libcmark.a)
