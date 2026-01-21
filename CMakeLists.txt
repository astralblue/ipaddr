cmake_minimum_required(VERSION 3.16)
project(ipaddr C)

set(CMAKE_C_STANDARD 11)

# Enable warnings
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# Source files
set(IPADDR_SOURCES
    main.c
    ipaddr_parse.c
    ipaddr_format.c
    ipaddr_uint128.c
    ipaddr_prefix.c
    ipaddr_classify.c
    ipaddr_network.c
    ipaddr_ipv6.c
    ipaddr_compare.c
)

add_executable(ipaddr ${IPADDR_SOURCES})

# Install rules
include(GNUInstallDirs)
install(TARGETS ipaddr DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ipaddr.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

# Enable testing
enable_testing()
add_test(NAME test_suite COMMAND ${CMAKE_SOURCE_DIR}/tests/test.sh)
set_tests_properties(test_suite PROPERTIES
    ENVIRONMENT "IPADDR=$<TARGET_FILE:ipaddr>"
)
