add_executable(diffu cli.cpp difftest.cpp gdbstub.cpp loader.cpp main.cpp)
target_link_libraries(diffu PRIVATE gdbstub spdlog::spdlog)
install ( TARGETS diffu )
