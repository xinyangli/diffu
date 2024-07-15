#include "api.hpp"
#include <cstdint>
#include <dlfcn.h>
#include <iostream>
#include <stdexcept>

Target::Target(const std::string &name, const std::string &func_prefix,
               const std::filesystem::path &path) {

  std::cout << path.c_str() << std::endl;
  meta = {.name = name,
          .libpath = path,
          .dlhandle = dlopen(path.c_str(), RTLD_LAZY)};

  if (!meta.dlhandle) {
    throw std::runtime_error(dlerror());
  }

#define LOAD_SYMBOL(ops, handle, prefix, name)                                 \
  do {                                                                         \
    ops.name = reinterpret_cast<decltype(DiffTargetApi::name)>(                \
        dlsym(handle, (prefix + #name).c_str()));                              \
    if (!ops.name)                                                             \
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

#undef LOAD_SYMBOL

  size_t *argsize_sym;
  argsize_sym = reinterpret_cast<size_t *>(dlsym(meta.dlhandle, "argsize"));
  if (!argsize_sym)
    goto load_error;

  argsize = *argsize_sym;
  args = std::vector<uint8_t>(argsize);

  arch_info_t *arch_sym;
  arch_sym =
      reinterpret_cast<arch_info_t *>(dlsym(meta.dlhandle, "isa_arch_info"));
  if (!arch_sym)
    goto load_error;
  return;

load_error:
  std::string err = std::string(dlerror());
  dlclose(meta.dlhandle);
  throw std::runtime_error(err);
}

Target::~Target() {
  std::cout << "Destruct target " << meta.name << std::endl;
  dlclose(meta.dlhandle);
}

bool Target::is_on_breakpoint() const { return is_on_breakpoint(last_res); }

bool Target::is_on_breakpoint(const gdb_action_t &res) const {
  if (res.reason == gdb_action_t::ACT_BREAKPOINT ||
      res.reason == gdb_action_t::ACT_RWATCH ||
      res.reason == gdb_action_t::ACT_WATCH ||
      res.reason == gdb_action_t::ACT_WWATCH) {
    return true;
  }
  return false;
}
