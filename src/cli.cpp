#include "config.hpp"
#include <CLI/App.hpp>
#include <CLI/Validators.hpp>

int Config::cli_parse(int argc, char **argv) {
  CLI::App app;
  app.add_option("-m,--memory", memory_file, "Content of memory")
      ->required()
      ->check(CLI::ExistingFile);

  app.add_option("--ref", refs, "Reference dynamic library")
      ->required()
      ->check(CLI::ExistingFile);

  app.add_option("--dut", dut, "Design under test")
      ->required()
      ->check(CLI::ExistingFile);

  app.set_config("-c,--config")
      ->transform(CLI::FileOnDefaultPath("./difftest.toml"));

  CLI11_PARSE(app, argc, argv);

  return 0;
}