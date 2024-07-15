#ifndef _DIFFTEST_DIFFTEST_H_
#define _DIFFTEST_DIFFTEST_H_
#include "api.hpp"
#include <filesystem>
#include <gdbstub.h>
#include <stdexcept>
#include <vector>

#include <iostream>
class Difftest {
private:
  Target dut;
  std::vector<Target> refs;

public:
  Difftest(Target &&dut, std::vector<Target> &&refs);

  void setup(const std::filesystem::path &memory_file);
  gdb_action_t stepi();
  gdb_action_t cont();
  static bool check(Target &dut, Target &ref) {
    for (int r = 0; r < dut.arch.reg_num; r++) {
      size_t regdut = 0, regref = 0;
      dut.ops.read_reg(dut.args.data(), r, &regdut);
      ref.ops.read_reg(ref.args.data(), r, &regref);
      if (regdut != regref) {
        std::cout << "reg: " << r << " dut: " << regdut << " ref: " << regref
                  << std::endl;
        throw std::runtime_error("Difftest failed");
      }
    }
    return true;
  };
  bool check_all();

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