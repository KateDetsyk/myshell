cmake_minimum_required(VERSION 3.10)
project(myshell)

set(CMAKE_CXX_STANDARD 14)
find_package(Boost COMPONENTS program_options filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

add_executable(myshell main.cpp build_in_commands.cpp utils.cpp)
target_link_libraries(myshell ${Boost_LIBRARIES} readline)

# additional commands
add_executable(mycat external_commands/mycat.cpp)
target_link_libraries(mycat ${Boost_LIBRARIES})

add_executable(myrls external_commands/myrls.cpp)
target_link_libraries(myrls ${Boost_LIBRARIES})

add_executable(mymkdir external_commands/mymkdir.cpp)
target_link_libraries(mymkdir ${Boost_LIBRARIES})

add_executable(mycp external_commands/mycp.cpp)
target_link_libraries(mycp ${Boost_LIBRARIES})

add_executable(myrm external_commands/myrm.cpp)
target_link_libraries(myrm ${Boost_LIBRARIES})