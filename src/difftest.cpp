#include "api.hpp"
#include <difftest.hpp>
#include <fstream>
#include <stdexcept>

#include <cstdio>

Difftest::Difftest(Target &&dut, std::vector<Target> &&refs) {
  this->dut = std::move(dut);
  this->refs = std::move(refs);

  for (const auto &ref : refs) {
    if (dut.arch.reg_byte != ref.arch.reg_byte ||
        dut.arch.reg_num != ref.arch.reg_num) {
      throw std::runtime_error("Ref and dut must have the same architecture");
    }
  }
}

void Difftest::setup(const std::filesystem::path &memory_file) {
  std::ifstream is = std::ifstream(memory_file, std::ios::binary);

  // Seek to the end to determine the file size
  is.seekg(0, std::ios::end);
  std::streampos memsize = is.tellg();
  is.seekg(0, std::ios::beg);

  std::vector<char> membuf(memsize);
  is.read(membuf.data(), memsize);
  is.close();

  // Initialize memory
  // TODO: reset vector should not be hardcoded
  // for(auto target : *this) {
  for (auto it = this->begin(); it != this->end(); ++it) {
    auto &target = *it;
    printf("init addr: %p\n", target.ops.init);
    target.ops.init(target.args.data());
    target.ops.write_mem(target.args.data(), 0x80000000UL, membuf.size(),
                         membuf.data());
    target.ops.write_reg(target.args.data(), 32, 0x80000000UL);
  }
}

bool Difftest::check_all() {
  for (auto &ref : refs) {
    check(dut, ref);
  }
  return true;
}

gdb_action_t Difftest::stepi() {
  bool breakflag = false;
  Target *pbreak;
  for (auto it = this->begin(); it != this->end(); ++it) {
    auto &target = *it;
    target.ops.stepi(target.args.data(), &target.last_res);
    if (target.is_on_breakpoint()) {
      breakflag = true;
      pbreak = &target;
    }
  }

  if (breakflag) {
    gdb_action_t ret = {.reason = gdb_action_t::ACT_BREAKPOINT};
    pbreak->ops.read_reg(pbreak->args.data(), 32, &ret.data);
    return ret;
  }
  return {gdb_action_t::ACT_NONE, 0};
}

gdb_action_t Difftest::cont() {
  bool breakflag = false;
  Target *pbreak;
  check_all();
  std::cerr << "setup finished." << std::endl;
  while (true) {
    // for(auto &target : *this) {
    for (auto it = this->begin(); it != this->end(); ++it) {
      auto &target = *it;
      target.ops.stepi(target.args.data(), &target.last_res);

      if (target.is_on_breakpoint()) {
        breakflag = true;
        pbreak = &target;
      }
    }

    check_all();

    if (breakflag) {
      gdb_action_t ret = {.reason = gdb_action_t::ACT_BREAKPOINT};
      pbreak->ops.read_reg(pbreak->args.data(), 32, &ret.data);
      return ret;
    }
  }
  return {gdb_action_t::ACT_NONE, 0};
}
