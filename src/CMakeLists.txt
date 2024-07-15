add_executable(diffu cli.cpp difftest.cpp loader.cpp main.cpp)
target_link_libraries(diffu PRIVATE gdbstub)
set_target_properties(diffu PROPERTIES ENABLE_EXPORTS 1)
