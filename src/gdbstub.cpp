#include <CLI/App.hpp>
#include <CLI/Error.hpp>
#include <cstring>
#include <difftest.hpp>
#include <sstream>
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

static void difftest_on_interrupt(void *args) {
  Difftest *diff = (Difftest *)args;
  puts("interrupt");
  diff->halt();
}

std::vector<std::string> split_into_args(const std::string &command) {
  std::istringstream iss(command);
  std::vector<std::string> args;
  std::string token;
  while (iss >> token) {
    args.push_back(token);
  }
  return args;
}

static char *gdbstub_monitor(void *args, const char *s) {
  Difftest *diff = (Difftest *)args;
  CLI::App parser;
  std::string ret = "";

  parser.add_subcommand("help", "Print help message")->callback([&]() {
    ret = parser.help();
  });
  auto sync = parser.add_subcommand("sync", "Sync states between targets")
                  ->callback([&]() { diff->sync_regs_to_ref(); });

  std::string cmdstr;
  int slen = strlen(s);
  int ch;
  for (int i = 0; i < slen; i += 2) {
    sscanf(&s[i], "%02x", &ch);
    cmdstr.push_back(ch);
  }

  auto arglist = split_into_args(cmdstr);
  std::vector<const char *> argv = {""};
  for (const auto &arg : arglist) {
    argv.push_back(static_cast<const char *>(arg.c_str()));
  }

  try {
    (parser).parse((argv.size()), (argv.data()));
  } catch (const CLI ::ParseError &e) {
    std::ostringstream os;
    os << "Failed to parse " << cmdstr << std::endl
       << parser.help() << std::endl;
    ret = os.str();
  }

  if (ret[0] == '\0') {
    return NULL;
  } else {
    std::ostringstream ret_stream;
    // Set formatting options for the stream
    ret_stream << std::hex << std::setfill('0');

    for (unsigned char c : ret) {
      ret_stream << std::setw(2) << static_cast<int>(c);
    }
    return strdup(ret_stream.str().c_str());
  }
}

int gdbstub_loop(Difftest *diff, std::string socket_addr) {
  target_ops gdbstub_ops = {.cont = difftest_cont,
                            .stepi = difftest_stepi,
                            .read_reg = difftest_read_reg,
                            .write_reg = difftest_write_reg,
                            .read_mem = difftest_read_mem,
                            .write_mem = difftest_write_mem,
                            .set_bp = difftest_set_bp,
                            .del_bp = difftest_del_bp,
                            .on_interrupt = difftest_on_interrupt,
                            .monitor = gdbstub_monitor};
  gdbstub_t gdbstub_priv;

  std::cout << "Waiting for gdb connection at " << socket_addr << std::endl;

  if (!gdbstub_init(&gdbstub_priv, &gdbstub_ops, diff->get_arch(),
                    socket_addr.c_str())) {
    std::cerr << "Failed to init socket at: " << socket_addr << std::endl;
    return false;
  }

  bool success = gdbstub_run(&gdbstub_priv, diff);
  gdbstub_close(&gdbstub_priv);
  return !success;
}
