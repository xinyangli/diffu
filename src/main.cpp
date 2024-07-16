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
  Target *dut = new Target{"dut", config.dut_prefix, config.dut};
  auto ref_libpath = config.refs.begin();
  auto ref_prefix = config.refs_prefix.begin();
  while (ref_libpath != config.refs.end() &&
         ref_prefix != config.refs_prefix.end()) {
    refs.emplace_back(ref_libpath->string(), *ref_prefix, *ref_libpath);
    ref_libpath++;
    ref_prefix++;
  }

  Difftest difftest{std::move(*dut), std::move(refs)};

  difftest.setup(config.memory_file);

  gdbstub_loop(&difftest);

  return 0;
}
