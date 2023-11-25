
add_executable(pp-serv pp-serv.cpp)
add_executable(pp-client pp-client.cpp)
add_executable(test-timer test-timer.cpp)

target_link_libraries(test-timer PUBLIC coio)
target_link_libraries(pp-serv PUBLIC coio)
target_link_libraries(pp-client PUBLIC coio)

