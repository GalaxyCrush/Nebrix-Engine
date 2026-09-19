add_executable(NebrixSandbox src/Main.cpp src/Maze.cpp src/MenuScene.cpp src/GameScene.cpp)

target_link_libraries(NebrixSandbox PRIVATE Nebrix)
target_compile_options(NebrixSandbox PRIVATE -Wall -Wextra -Wpedantic)
