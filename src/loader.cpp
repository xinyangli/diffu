#include "api.hpp"
#include <cstdint>
#include <dlfcn.h>
#include <gdbstub.h>
#include <stdexcept>

Target::Target(const std::string &name, const std::string &func_prefix,
               const std::filesystem::path &path) {

  meta = {.name = name,
          .libpath = path,
          .dlhandle = dlopen(path.c_str(), RTLD_NOW)};

  if (!meta.dlhandle) {
    throw std::runtime_error(dlerror());
  }

#define LOAD_SYMBOL(ops, handle, prefix, name)                                 \
  do {                                                                         \
    (ops).name = reinterpret_cast<decltype((ops).name)>(                       \
        dlsym((handle), ((prefix) + #name).c_str()));                          \
    if (!((ops).name))                                                         \
      goto load_error;                                                         \
  } while (0);

  LOAD_SYMBOL(ops, meta.dlhandle, func_prefix, cont);
  LOAD_SYMBOL(ops, meta.dlhandle, func_prefix, stepi);
  LOAD_SYMBOL(ops, meta.dlhandle, func_prefix, read_reg);
  LOAD_SYMBOL(ops, meta.dlhandle, func_prefix, write_reg);
  LOAD_SYMBOL(ops, meta.dlhandle, func_prefix, read_mem);
  LOAD_SYMBOL(ops, meta.dlhandle, func_prefix, write_mem);
  LOAD_SYMBOL(ops, meta.dlhandle, func_prefix, set_bp);
  LOAD_SYMBOL(ops, meta.dlhandle, func_prefix, del_bp);
  LOAD_SYMBOL(ops, meta.dlhandle, func_prefix, on_interrupt);
  LOAD_SYMBOL(ops, meta.dlhandle, func_prefix, init);

  LOAD_SYMBOL(*this, meta.dlhandle, func_prefix, do_difftest);
  *do_difftest = true;

  LOAD_SYMBOL(*this, meta.dlhandle, func_prefix, dbg_state_size);
  args.resize(*dbg_state_size);
  LOAD_SYMBOL(*this, meta.dlhandle, func_prefix, isa_arch_info);
#undef LOAD_SYMBOL

  return;

load_error:
  std::string err = std::string(dlerror());
  dlclose(meta.dlhandle);
  throw std::runtime_error(err);
}

Target::~Target() { dlclose(meta.dlhandle); }

bool Target::is_on_breakpoint() const { return is_on_breakpoint(last_res); }

bool Target::is_on_breakpoint(const gdb_action_t &res) const {
  if (res.reason == gdb_action_t::ACT_BREAKPOINT ||
      res.reason == gdb_action_t::ACT_RWATCH ||
      res.reason == gdb_action_t::ACT_WATCH ||
      res.reason == gdb_action_t::ACT_WWATCH ||
      res.reason == gdb_action_t::ACT_SHUTDOWN) {
    return true;
  }
  return false;
}
