#ifndef _DIFFTEST_API_H_
#define _DIFFTEST_API_H_

#include <cstddef>

extern "C" {

typedef struct {
  enum {
    ACT_NONE,
    ACT_BREAKPOINT,
    ACT_WATCH,
    ACT_RWATCH,
    ACT_WWATCH,
    ACT_SHUTDOWN
  } reason;
  size_t data;
} gdb_action_t;

typedef enum { BP_SOFTWARE = 0, BP_WRITE, BP_READ, BP_ACCESS } bp_type_t;

struct target_ops {
  void (*cont)(void *args, gdb_action_t *res);
  void (*stepi)(void *args, gdb_action_t *res);
  int (*read_reg)(void *args, int regno, size_t *value);
  int (*write_reg)(void *args, int regno, size_t value);
  int (*read_mem)(void *args, size_t addr, size_t len, void *val);
  int (*write_mem)(void *args, size_t addr, size_t len, void *val);
  bool (*set_bp)(void *args, size_t addr, bp_type_t type);
  bool (*del_bp)(void *args, size_t addr, bp_type_t type);
  void (*on_interrupt)(void *args);
};
}

#endif