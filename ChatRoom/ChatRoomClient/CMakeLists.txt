cmake_minimum_required(VERSION 3.10)

project(ChatRoomClient)

set(CMAKE_CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

set(SRC_LIST
    src/ChatRoomClient.cpp
    src/main.cpp
)

add_executable(${PROJECT_NAME} ${SRC_LIST})

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/head)

target_link_libraries(${PROJECT_NAME} PRIVATE pthread)

