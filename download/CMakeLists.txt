cmake_minimum_required(VERSION 3.10)
project(file_sharing C)

set(CMAKE_C_STANDARD 11)

# Find required packages
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONC REQUIRED json-c)
pkg_check_modules(LIBZIP REQUIRED libzip)

# Link libraries
link_libraries(${JSONC_LIBRARIES} mysqlclient)

file(GLOB HANDLER_SOURCES "src/server/handler/*.c")

# Create executables
add_executable(server
    src/server/db/db_access.c
    src/server/system/system_access.c
    src/server/server.c
    ${HANDLER_SOURCES} # Add handler sources
    src/utils/helper.c
    src/utils/structs.c
)
add_executable(client src/client.c src/utils/helper.c src/server/system/system_access.c)

add_executable(test src/test.c)

# Add a custom target for formatting
add_custom_target(format
    COMMAND find ${CMAKE_SOURCE_DIR} -name '*.c' -o -name '*.h' | xargs clang-format -i
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Formatting all source files"
)

# Add include directories
include_directories(${JSONC_INCLUDE_DIRS})

# Update link libraries
target_link_libraries(server ${JSONC_LIBRARIES} ${LIBZIP_LIBRARIES} mysqlclient pthread)
target_link_libraries(client ${JSONC_LIBRARIES} ${LIBZIP_LIBRARIES} pthread)
target_link_libraries(test ${LIBZIP_LIBRARIES})
