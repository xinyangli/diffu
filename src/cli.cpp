#include "config.hpp"
#include <CLI/App.hpp>
#include <CLI/Error.hpp>
#include <CLI/Validators.hpp>
#include <cerrno>

int Config::cli_parse(int argc, char **argv) {
  CLI::App app;
  app.add_option("-m,--memory", memory_file, "Content of memory")
      ->required()
      ->check(CLI::ExistingFile);

  app.add_option("--ref", refs, "Reference dynamic library")
      ->required()
      ->check(CLI::ExistingFile);

  app.add_option("--ref-prefix", refs_prefix,
                 "Optional prefix for each reference library");

  app.add_option("--dut", dut, "Design under test")
      ->required()
      ->check(CLI::ExistingFile);

  app.add_option("--dut-prefix", dut_prefix,
                 "Optional prefix for design under test");

  app.add_option("--listen", gdbstub_addr, "Gdb remote listen address");

  app.add_flag("-g", use_debugger, "Launch gdb remote stub");

  app.set_config("-c,--config")
      ->transform(CLI::FileOnDefaultPath("difftest.toml"));

  // Default value for refs_prefix
  app.callback([&]() {
    if (refs_prefix.size() == 0) {
      refs_prefix.insert(refs_prefix.end(), refs.size(), "");
    }
  });

  // Check if refs_prefix matches refs.
  app.callback([&]() {
    if (refs_prefix.size() != refs.size()) {
      throw CLI::ParseError(
          "Same number of --ref and --ref-prefix must be provided.", EINVAL);
    }
  });

  CLI11_PARSE(app, argc, argv);

  return 0;
}
