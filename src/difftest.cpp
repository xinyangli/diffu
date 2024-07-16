#include "api.hpp"
#include <difftest.hpp>
#include <fstream>
#include <gdbstub.h>
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

bool Difftest::exec(size_t n, gdb_action_t *ret) {
  while (n--) {
    bool breakflag = false;
    Target *pbreak = &(*(this->begin()));
    for (auto it = this->begin(); it != this->end(); ++it) {
      auto &target = *it;
      target.ops.stepi(target.args.data(), &target.last_res);
      if (target.is_on_breakpoint()) {
        breakflag = true;
        pbreak = &target;
      }
    }

    if (breakflag) {
      ret->reason = pbreak->last_res.reason;
      ret->data = pbreak->last_res.data;
      return false;
    }
  }
  return true;
}

gdb_action_t Difftest::stepi() {
  gdb_action_t ret = {.reason = gdb_action_t::ACT_NONE};
  exec(1, &ret);
  check_all();
  return ret;
}

gdb_action_t Difftest::cont() {
  gdb_action_t ret = {.reason = gdb_action_t::ACT_NONE};
  while (exec(1, &ret)) {
    check_all();
  };
  return ret;
}

int Difftest::read_reg(int regno, size_t *value) {
  return current_target->ops.read_reg(current_target->args.data(), regno,
                                      value);
}

int Difftest::write_reg(int regno, size_t value) {
  return current_target->ops.write_reg(current_target->args.data(), regno,
                                       value);
}

int Difftest::read_mem(size_t addr, size_t len, void *val) {
  return current_target->ops.read_mem(current_target->args.data(), addr, len,
                                      val);
}

int Difftest::write_mem(size_t addr, size_t len, void *val) {
  return current_target->ops.write_mem(current_target->args.data(), addr, len,
                                       val);
}

bool Difftest::set_bp(size_t addr, bp_type_t type) {
  bool ret = true;
  for (auto it = this->begin(); it != this->end(); ++it) {
    auto &target = *it;
    ret = target.ops.set_bp(target.args.data(), addr, type) && ret;
  }
  return ret;
}

bool Difftest::del_bp(size_t addr, bp_type_t type) {
  bool ret = true;
  for (auto it = this->begin(); it != this->end(); ++it) {
    auto &target = *it;
    ret = target.ops.del_bp(target.args.data(), addr, type) && ret;
  }
  return ret;
}
