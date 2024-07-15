#ifndef _DIFFTEST_API_H_
#define _DIFFTEST_API_H_

#include <cstddef>
#include <filesystem>
#include <gdbstub.h>
#include <string>
#include <vector>

// Target dynamic library has to implement these functions
struct DiffTargetApi {
  typedef void (*cont_t)(void *args, gdb_action_t *res);
  cont_t cont;

  typedef void (*stepi_t)(void *args, gdb_action_t *res);
  stepi_t stepi;

  typedef int (*read_reg_t)(void *args, int regno, size_t *value);
  read_reg_t read_reg;

  typedef int (*write_reg_t)(void *args, int regno, size_t value);
  write_reg_t write_reg;

  typedef int (*read_mem_t)(void *args, size_t addr, size_t len, void *val);
  read_mem_t read_mem;

  typedef int (*write_mem_t)(void *args, size_t addr, size_t len, void *val);
  write_mem_t write_mem;

  typedef bool (*set_bp_t)(void *args, size_t addr, bp_type_t type);
  set_bp_t set_bp;

  typedef bool (*del_bp_t)(void *args, size_t addr, bp_type_t type);
  del_bp_t del_bp;

  typedef void (*on_interrupt_t)(void *args);
  on_interrupt_t on_interrupt;

  typedef void (*init_t)(void *args);
  init_t init;
};

struct TargetMeta {
  std::string name;
  std::filesystem::path libpath;
  void *dlhandle;
};

class Target {
public:
  DiffTargetApi ops;
  TargetMeta meta;
  arch_info_t arch;
  size_t argsize;
  std::vector<uint8_t> args; // used as a buffer to store target specific values

  gdb_action_t last_res;

  Target(){};
  Target(const std::string &name, const std::string &prefix,
         const std::filesystem::path &path);
  ~Target();

  bool is_on_breakpoint() const;
  bool is_on_breakpoint(const gdb_action_t &res) const;
};

#endif