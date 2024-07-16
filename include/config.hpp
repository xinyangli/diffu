#include <CLI/App.hpp>
#include <CLI/CLI.hpp>
#include <CLI/Validators.hpp>
#include <filesystem>
#include <vector>

struct Config {
  std::filesystem::path memory_file;
  std::vector<std::filesystem::path> refs;
  std::vector<std::string> refs_prefix;
  std::filesystem::path dut;
  std::string dut_prefix = "";

  int cli_parse(int argc, char **argv);
};

extern Config config;
