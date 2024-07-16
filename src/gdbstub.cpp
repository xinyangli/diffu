#include <difftest.hpp>
extern "C" {
#include <gdbstub.h>
}

static void difftest_cont(void *args, gdb_action_t *res) {
  Difftest *diff = (Difftest *)args;
  *res = diff->cont();
};

static void difftest_stepi(void *args, gdb_action_t *res) {
  Difftest *diff = (Difftest *)args;
  *res = diff->stepi();
};

static int difftest_read_reg(void *args, int regno, size_t *value) {
  Difftest *diff = (Difftest *)args;
  return diff->read_reg(regno, value);
};

static int difftest_write_reg(void *args, int regno, size_t value) {
  Difftest *diff = (Difftest *)args;
  return diff->write_reg(regno, value);
}

static int difftest_read_mem(void *args, size_t addr, size_t len, void *val) {
  Difftest *diff = (Difftest *)args;
  return diff->read_mem(addr, len, val);
}

static int difftest_write_mem(void *args, size_t addr, size_t len, void *val) {
  Difftest *diff = (Difftest *)args;
  return diff->write_mem(addr, len, val);
}

static bool difftest_set_bp(void *args, size_t addr, bp_type_t type) {
  Difftest *diff = (Difftest *)args;
  return diff->set_bp(addr, type);
}

static bool difftest_del_bp(void *args, size_t addr, bp_type_t type) {
  Difftest *diff = (Difftest *)args;
  return diff->del_bp(addr, type);
}

int gdbstub_loop(Difftest *diff) {
  target_ops gdbstub_ops = {.cont = difftest_cont,
                            .stepi = difftest_stepi,
                            .read_reg = difftest_read_reg,
                            .write_reg = difftest_write_reg,
                            .read_mem = difftest_read_mem,
                            .write_mem = difftest_write_mem,
                            .set_bp = difftest_set_bp,
                            .del_bp = difftest_del_bp,
                            .on_interrupt = NULL};
  gdbstub_t gdbstub_priv;
  char socket_addr[] = "127.0.0.1:1234";
  gdbstub_init(&gdbstub_priv, &gdbstub_ops, diff->get_arch(), socket_addr);

  std::cout << "Waiting for gdb connection at " << socket_addr << std::endl;
  bool success = gdbstub_run(&gdbstub_priv, diff);
  gdbstub_close(&gdbstub_priv);
  return !success;
}