#include "api.hpp"
#include <cerrno>
#include <difftest.hpp>
#include <fstream>
#include <gdbstub.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdexcept>
#include <string>

Difftest::Difftest(Target &&dut, std::vector<Target> &&refs) {
  this->dut = std::move(dut);
  this->refs = std::move(refs);

  for (const auto &ref : refs) {
    if (dut.isa_arch_info->reg_byte != ref.isa_arch_info->reg_byte ||
        dut.isa_arch_info->reg_num != ref.isa_arch_info->reg_num) {
      throw std::runtime_error("Ref and dut must have the same architecture");
    }
  }
}

void Difftest::setup(const std::filesystem::path &memory_file) {
  std::ifstream is = std::ifstream(memory_file, std::ios::binary);

  spdlog::debug("Reading image file: {}", memory_file.c_str());
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
    target.ops.write_reg(target.args.data(), 32, 0x80000000UL);
    if (target.ops.write_mem(target.args.data(), 0x80000000UL, membuf.size(),
                             membuf.data()) != 0)
      throw std::runtime_error("write_mem failed");
  }
}

bool Difftest::check_all() {
  bool passed = true;
  for (auto &ref : refs) {
    if (check(dut, ref) == false)
      passed = false;
  }
  return passed;
}

Difftest::ExecRet Difftest::exec(size_t n, gdb_action_t *ret) {
  ExecRet exec_ret = {.at_breakpoint = false, .do_difftest = true};
  while (n--) {
    Target *pbreak = &(*(this->begin()));
    // TODO: For improvement, use ThreadPool here for concurrent execution?
    for (auto it = this->begin(); it != this->end(); ++it) {
      auto &target = *it;
      *target.do_difftest = true;
      target.ops.stepi(target.args.data(), &target.last_res);
      spdlog::trace("{} stepped once", target.meta.name);
      if (target.is_on_breakpoint()) {
        exec_ret.at_breakpoint = true;
        pbreak = &target;
        spdlog::trace("{} on breakpoint", target.meta.name);
        continue;
      }
      exec_ret.do_difftest = *target.do_difftest && exec_ret.do_difftest;
    }

    if (exec_ret.at_breakpoint) {
      ret->reason = pbreak->last_res.reason;
      ret->data = pbreak->last_res.data;
      break;
    }
  }
  return exec_ret;
}

gdb_action_t Difftest::stepi() {
  gdb_action_t ret = {.reason = gdb_action_t::ACT_NONE};
  ExecRet exec_result = exec(1, &ret);
  if (exec_result.do_difftest) {
    check_all();
  } else {
    sync_regs_to_ref();
  }
  return ret;
}

gdb_action_t Difftest::cont() {
  gdb_action_t ret = {.reason = gdb_action_t::ACT_NONE};
  ExecRet exec_ret;
  start_run();
  while (!is_halt()) {
    exec_ret = exec(1, &ret);
    if (exec_ret.do_difftest) {
      check_all();
    } else {
      size_t pc = 0;
      read_reg(32, &pc);
      spdlog::debug("Difftest skipped at {}", (void *)pc);
      sync_regs_to_ref();
    }
    if (exec_ret.at_breakpoint)
      break;
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

int Difftest::sync_regs_to_ref(void) {
  std::vector<size_t> regs;
  int ret = 0;
  for (int i = 0; i <= get_arch().reg_num; i++) {
    size_t r;
    ret = dut.ops.read_reg(dut.args.data(), i, &r);
    if (ret)
      return ret;
    regs.push_back(r);
  }
  for (auto &ref : refs) {
    for (int i = 0; i <= get_arch().reg_num; i++) {
      ret = ref.ops.write_reg(ref.args.data(), i, regs.at(i));
      if (ret)
        return ret;
    }
  }
  spdlog::trace("Applied registers value from dut to ref");
  return ret;
}

std::string Difftest::list_targets(void) {
  std::ostringstream os;
  int i = 0;
  for (auto it = this->begin(); it != this->end(); ++it, ++i) {
    auto &target = *it;
    os << i << ": " << target.meta.name << std::endl;
  }
  os << "current: " << current_target->meta.name << std::endl;
  return os.str();
}

std::string Difftest::switch_target(int index) {
  std::ostringstream os;
  int i = 0;
  for (auto it = this->begin(); it != this->end(); ++it, ++i) {
    auto &target = *it;
    if (i == index) {
      current_target = &target;
      os << "Switched to " << current_target->meta.name << std::endl;
      return os.str();
    }
  }
  os << "Invalid target target index: " << index << std::endl;
  return os.str();
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
