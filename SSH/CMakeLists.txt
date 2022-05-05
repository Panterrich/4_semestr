#########################################################################

# CMAKE MINIMAL VERSION
cmake_minimum_required (VERSION 3.11)

# PROJECT PROPERTIES
project ("SSH" LANGUAGES C)

#########################################################################

# COMPILE OPTIONS
add_compile_options(
    -Wall

    # -Wno-pointer-to-int-cast
    # -Wno-int-to-pointer-cast
    # -Wno-variadic-macros
    # -Wno-unused-variable
    # -Wno-unused-parameter
    # -Wno-unused-function
    # -Wno-sign-compare
    # -Wno-misleading-indentation
    # -Wno-format
)

set(DEBUG_COMPILER_OPTIONS
    -g -O1 -DDBG
)

set(RELEASE_COMPILER_OPTIONS
    -O2
)

#########################################################################

# EXECUTABLE FILES PROPERTIES
set(SSH_CLIENT_NAME "ssh_client")
set(SSH_SERVER_NAME "ssh_server")
set(SSH_SERVER_CONTROLLER_NAME "ssh_controller")

set(SSH_CLIENT_SRC
    src/ssh_client/main.c
    src/ssh_client/ssh_client.c
)  

set(SSH_SERVER_SRC
    src/ssh_server/ssh_server.c
    src/ssh_server/daemon.c
    src/ssh_server/main.c
    
)   

set(SSH_SERVER_CONTROLLER_SRC
    src/ssh_server_controller/controller.c
    src/ssh_server_controller/main.c
)   

add_library(rudp STATIC src/pty/pty.c src/rudp/rudp.c src/pty/pam.c)

# EXECUTABLE FILES
add_executable(${SSH_CLIENT_NAME}            ${SSH_CLIENT_SRC})
add_executable(${SSH_SERVER_NAME}            ${SSH_SERVER_SRC})
add_executable(${SSH_SERVER_CONTROLLER_NAME} ${SSH_SERVER_CONTROLLER_SRC})

target_link_libraries(${SSH_CLIENT_NAME} rudp pam pam_misc pthread)
target_link_libraries(${SSH_SERVER_NAME} rudp pam pam_misc pthread)

target_include_directories(${SSH_CLIENT_NAME}            PRIVATE src/logger src/include src/hashtable src/pty src/rudp)
target_include_directories(${SSH_SERVER_NAME}            PRIVATE src/logger src/include src/hashtable src/pty src/rudp)
target_include_directories(${SSH_SERVER_CONTROLLER_NAME} PRIVATE src/logger src/include src/hashtable src/pty src/rudp)

#########################################################################

foreach(TARGET IN LISTS TARGETS)

    option(DEBUG_BUILD "Build debug project version." ON)

    if (DEBUG_BUILD)
        target_compile_options(${TARGET} PRIVATE ${DEBUG_COMPILER_OPTIONS})
    else()
        target_compile_options(${TARGET} PRIVATE ${RELEASE_COMPILER_OPTIONS})
    endif (DEBUG_BUILD)

endforeach()

#########################################################################