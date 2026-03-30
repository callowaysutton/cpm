
#include "../include/cli.h"
#include "test_util.h"
#include <sht.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

SHT_SUITE(CLI)

SHT_TEST(CLI, basic_commands) {
  int sout, serr;
  // Just call cpm_dispatch but skip commands that modify filesystem states
  // directly

  // Test missing command
  char *args_missing[] = {"cpm", NULL};
  cpm_log_begin("cli", &sout, &serr);
  int r1 = cpm_dispatch(1, args_missing);
  cpm_log_end(&sout, &serr);
  SHT_ASSERT_EQ(r1, EXIT_FAILURE);

  // Test unknown command
  char *args_unknown[] = {"cpm", "blablabla", NULL};
  cpm_log_begin("cli", &sout, &serr);
  int r2 = cpm_dispatch(2, args_unknown);
  cpm_log_end(&sout, &serr);
  SHT_ASSERT_EQ(r2, EXIT_FAILURE);

  // Test help command
  char *args_help[] = {"cpm", "help", NULL};
  cpm_log_begin("cli", &sout, &serr);
  int r3 = cpm_dispatch(2, args_help);
  cpm_log_end(&sout, &serr);
  SHT_ASSERT_EQ(r3, EXIT_SUCCESS);
}

SHT_TEST(CLI, get_command_errors) {
  int sout, serr;
  // Missing args
  char *args_get1[] = {"cpm", "get", NULL};
  cpm_log_begin("cli", &sout, &serr);
  int r1 = cpm_dispatch(2, args_get1);
  cpm_log_end(&sout, &serr);
  SHT_ASSERT_EQ(r1, EXIT_FAILURE);

  // Bad arg format
  char *args_get2[] = {"cpm", "get", "no_at_symbol_here", NULL};
  cpm_log_begin("cli", &sout, &serr);
  int r2 = cpm_dispatch(3, args_get2);
  cpm_log_end(&sout, &serr);
  SHT_ASSERT_EQ(r2, EXIT_FAILURE);
}

SHT_TEST(CLI, get_vendor_tidy_flow) {
  int sout, serr;
  // Preserve existing workspace
  sys_log("cli", "mv .cpm .cpm_backup_test_cli 2>/dev/null");
  sys_log("cli", "mv cmod.toml cmod.toml_backup_test_cli 2>/dev/null");

  // Test init
  char *args_init[] = {"cpm", "init", NULL};
  cpm_log_begin("cli", &sout, &serr);
  int r_init = cpm_dispatch(2, args_init);
  cpm_log_end(&sout, &serr);
  SHT_ASSERT_EQ(r_init, EXIT_SUCCESS);
  SHT_ASSERT_EQ(system("test -f cmod.toml"), 0);

  // Test get latest tag
  char arg2[] = "github.com/callowaysutton/sht-lib@v1.0.0";
  char *args_get[] = {"cpm", "get", arg2, NULL};
  cpm_log_begin("cli", &sout, &serr);
  int r_get = cpm_dispatch(3, args_get);
  cpm_log_end(&sout, &serr);
  SHT_ASSERT_EQ(r_get, EXIT_SUCCESS);

  sys_log("cli", "cat cmod.toml");

  // Test vendor
  char *args_vendor[] = {"cpm", "vendor", NULL};
  cpm_log_begin("cli", &sout, &serr);
  int r_vendor = cpm_dispatch(2, args_vendor);
  cpm_log_end(&sout, &serr);
  SHT_ASSERT_EQ(r_vendor, EXIT_SUCCESS);

  sys_log("cli", "mkdir -p .cpm/vendor/dummy_directory_to_delete");

  // Test tidy
  char *args_tidy[] = {"cpm", "tidy", NULL};
  cpm_log_begin("cli", &sout, &serr);
  int r_tidy = cpm_dispatch(2, args_tidy);
  cpm_log_end(&sout, &serr);
  SHT_ASSERT_EQ(r_tidy, EXIT_SUCCESS);

  // Test clean
  char *args_clean[] = {"cpm", "clean", NULL};
  cpm_log_begin("cli", &sout, &serr);
  int r_clean = cpm_dispatch(2, args_clean);
  cpm_log_end(&sout, &serr);
  SHT_ASSERT_EQ(r_clean, EXIT_SUCCESS);

  // Restore existing workspace
  sys_log("cli", "rm -rf .cpm cmod.toml");
  sys_log("cli", "mv .cpm_backup_test_cli .cpm 2>/dev/null");
  sys_log("cli", "mv cmod.toml_backup_test_cli cmod.toml 2>/dev/null");
}

SHT_TEST(CLI, build_command_flow) {
  int sout, serr;
  sys_log("cli", "mv .cpm .cpm_backup_test_cli_build 2>/dev/null");
  sys_log("cli", "mv cmod.toml cmod.toml_backup_test_cli_build 2>/dev/null");

  char *args_init[] = {"cpm", "init", NULL};
  cpm_log_begin("cli", &sout, &serr);
  int r_init = cpm_dispatch(2, args_init);
  cpm_log_end(&sout, &serr);
  SHT_ASSERT_EQ(r_init, EXIT_SUCCESS);

  sys_log("cli", "echo '\n[build]\ncommand = \"true\"' >> cmod.toml");

  char *args_build[] = {"cpm", "build", NULL};
  cpm_log_begin("cli", &sout, &serr);
  int r_build = cpm_dispatch(2, args_build);
  cpm_log_end(&sout, &serr);
  SHT_ASSERT_EQ(r_build, EXIT_SUCCESS);

  sys_log("cli", "rm -rf .cpm cmod.toml");
  sys_log("cli", "mv .cpm_backup_test_cli_build .cpm 2>/dev/null");
  sys_log("cli", "mv cmod.toml_backup_test_cli_build cmod.toml 2>/dev/null");
}
