CMAKE_MINIMUM_REQUIRED(VERSION 3.13)
CMAKE_POLICY(SET CMP0048 NEW)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g -Wall -Wextra  ")
PROJECT(chatnet VERSION 2 DESCRIPTION "real-time client to chatnet-server" HOMEPAGE_URL "https://github.com/midnqp/chatnet-cli" LANGUAGES C)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/deps)

#ADD_LIBRARY(util SHARED ./util.c) 
#ADD_LIBRARY(string SHARED ./string.c) 
#ADD_LIBRARY(sioclient SHARED ./sio-client.c) 
#ADD_LIBRARY(autofree SHARED ./autofree.c) 
#ADD_LIBRARY(db SHARED ./db.c) 
#ADD_LIBRARY(linenoise SHARED ./deps/linenoise/linenoise.c)
#ADD_LIBRARY(sc SHARED ./deps/sc/sc_log.c)

ADD_LIBRARY(libraries STATIC ./util.c ./string.c ./sio-client.c ./autofree.c ./db.c)
ADD_LIBRARY(deps STATIC ./deps/linenoise/linenoise.c ./deps/sc/sc_log.c)


ADD_EXECUTABLE(chatnet client.c)


TARGET_LINK_LIBRARIES(chatnet libraries deps json-c uuid )
#TARGET_LINK_LIBRARIES(chatnet sioclient util autofree string  db json-c uuid linenoise sc)


#TARGET_COMPILE_OPTIONS(chatnet PRIVATE -Wall -fPIC -pie -Wextra -g -O0 -fsanitize=thread )
#TARGET_LINK_OPTIONS(chatnet PRIVATE -Wall -fPIC -pie -Wextra -g  -O0 -fsanitize=thread )
