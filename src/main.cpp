#include "api.hpp"
#include "config.hpp"
#include "difftest.hpp"

// extern "C" {
int gdbstub_loop(Difftest *);
// }
int main(int argc, char **argv) {
  Config config;
  int ret = 0;
  ret = config.cli_parse(argc, argv);
  if (ret)
    return ret;

  std::vector<Target> refs;
  Target *dut = new Target{"dut", "nemu_", config.dut};
  for (const auto &ref_libpath : config.refs) {
    refs.emplace_back(ref_libpath.string(), "nemu_", ref_libpath);
  }

  Difftest difftest{std::move(*dut), std::move(refs)};

  difftest.setup(config.memory_file);

  gdbstub_loop(&difftest);

  return 0;
}
