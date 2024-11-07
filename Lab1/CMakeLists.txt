cmake_minimum_required(VERSION 3.10)

project(LAB1 C)

add_executable(server server.c)
add_executable(client client.c)

# Link with ws2_32 library for Winsock functions
target_link_libraries(server ws2_32)
target_link_libraries(client ws2_32)
