#include "api.hpp"
#include "config.hpp"
#include "difftest.hpp"
#include <filesystem>
#include <spdlog/cfg/env.h>
#include <spdlog/spdlog.h>

int gdbstub_loop(Difftest *, std::string);

int main(int argc, char **argv) {
  spdlog::cfg::load_env_levels();
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

  std::filesystem::path image_file = config.images_path / config.memory_file;
  if (!std::filesystem::exists(image_file)) {
    spdlog::error("Cannot find {} in {}.", config.memory_file.c_str(),
                  config.images_path.c_str());
  }
  difftest.setup(image_file);

  if (config.use_debugger) {
    gdbstub_loop(&difftest, config.gdbstub_addr);
  } else {
    difftest.cont();
  }

  return 0;
}
