#ifndef _DIFFTEST_DIFFTEST_H_
#define _DIFFTEST_DIFFTEST_H_
#include "api.hpp"
#include <filesystem>
#include <spdlog/spdlog.h>
#include <vector>
extern "C" {
#include <gdbstub.h>
}

class Difftest {
private:
  Target dut;
  std::vector<Target> refs;

  // target used for read_reg, write_reg, read_mem, write_mem
  Target *current_target = &dut;
  bool halt_status = false;
  inline void start_run() {
    __atomic_store_n(&halt_status, false, __ATOMIC_RELAXED);
  };
  inline bool is_halt() {
    return __atomic_load_n(&halt_status, __ATOMIC_RELAXED);
  };

  struct ExecRet {
    bool at_breakpoint;
    bool do_difftest;
  };
  ExecRet exec(size_t n, gdb_action_t *ret);

public:
  Difftest(Target &&dut, std::vector<Target> &&refs);

  void setup(const std::filesystem::path &memory_file);

  // Export API for gdbstub
  gdb_action_t cont();
  gdb_action_t stepi();
  int read_reg(int regno, size_t *value);
  int write_reg(int regno, size_t value);
  int read_mem(size_t addr, size_t len, void *val);
  int write_mem(size_t addr, size_t len, void *val);
  bool set_bp(size_t addr, bp_type_t type);
  bool del_bp(size_t addr, bp_type_t type);

  bool check_all();
  int sync_regs_to_ref(void);
  std::string list_targets(void);
  std::string switch_target(int index);

  inline void halt() {
    __atomic_store_n(&halt_status, true, __ATOMIC_RELAXED);
  };

  arch_info_t get_arch() const { return *dut.isa_arch_info; }

  static bool check(Target &dut, Target &ref) {
    size_t pcref, pcdut;
    bool passed = true;
    dut.ops.read_reg(dut.args.data(), 32, &pcdut);
    ref.ops.read_reg(ref.args.data(), 32, &pcref);
    for (int r = 0; r < dut.isa_arch_info->reg_num; r++) {
      size_t regdut = 0, regref = 0;
      dut.ops.read_reg(dut.args.data(), r, &regdut);
      ref.ops.read_reg(ref.args.data(), r, &regref);
      if (regdut != regref) {
        spdlog::error("Reg {} different: \n\tPC:\t(ref) {:x}\t(dut) {:x}\n\t"
                      "value:\t(ref) {:x}\t(dut) {:x}",
                      r, pcref, pcdut, regref, regdut);
        passed = false;
      }
    }
    return passed;
  };

  class Iterator {
  private:
    Difftest &difftest;
    size_t index;
    bool on_dut;

  public:
    Iterator(Difftest &difftest, size_t index, bool on_dut)
        : difftest(difftest), index(index), on_dut(on_dut) {}

    Iterator &operator++() {
      if (on_dut) {
        on_dut = false;
      } else {
        ++index;
      }
      return *this;
    }

    bool operator!=(const Iterator &other) const {
      return index != other.index || on_dut != other.on_dut;
    }

    Target &operator*() {
      if (on_dut) {
        return difftest.dut;
      } else {
        return difftest.refs.at(index);
      }
    }
  };

  Iterator begin() { return Iterator(*this, 0, true); }

  Iterator end() { return Iterator(*this, refs.size(), false); }
};

#endif
