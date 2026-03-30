#include "../include/vendor.h"
#include "test_util.h"
#include <sht.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

SHT_SUITE(Vendor)

static void vendor_setup(void) {
  sys_log("vendor", "mkdir -p .cpm/cache/dummy_repo@v1.0.0/src");
  sys_log("vendor", "echo 'int foo() { return 1; }' > "
         ".cpm/cache/dummy_repo@v1.0.0/src/foo.c");
  sys_log("vendor", "mkdir -p .cpm/cache/dummy_repo@v1.0.0/.git");
}

static void vendor_teardown(void) {
  sys_log("vendor", "rm -rf .cpm/cache/dummy_repo@v1.0.0");
  sys_log("vendor", "rm -rf .cpm/vendor/dummy_repo@v1.0.0");
}

SHT_TEST(Vendor, populate_and_clean) {
  int sout, serr;
  vendor_setup();

  resolved_t list[1];
  list[0].module = "dummy/repo";
  list[0].version = "v1.0.0";

  // Populate should copy the file over
  cpm_log_begin("vendor", &sout, &serr);
  int rc = vendor_populate(list, 1);
  cpm_log_end(&sout, &serr);

  sys_log("vendor", "ls -laR .cpm/vendor/dummy_repo@v1.0.0 > vendor_debug.txt 2>&1");
  sys_log("vendor", "ls -laR .cpm/cache/dummy_repo@v1.0.0 >> vendor_debug.txt 2>&1");

  // Verify it was copied
  SHT_ASSERT_EQ(system("test -f .cpm/vendor/dummy_repo@v1.0.0/src/foo.c"), 0);

  // Now test clean unused
  cpm_log_begin("vendor", &sout, &serr);
  rc = vendor_clean_unused("dummy/repo", "v1.0.0");
  cpm_log_end(&sout, &serr);
  SHT_ASSERT_EQ(rc, 0);

  // Verify it was cleaned
  SHT_ASSERT_NE(system("test -f .cpm/vendor/dummy_repo@v1.0.0/src/foo.c"), 0);

  vendor_teardown();
}

SHT_TEST(Vendor, populate_errors) {
  int sout, serr;
  vendor_setup();

  // Pass a fake module that doesn't exist in cache to test failed copy
  // directory
  resolved_t list[1];
  list[0].module = "fake/repo";
  list[0].version = "v2.0.0";

  // Should return 0 from vendor_populate because it just warns on stderr
  cpm_log_begin("vendor", &sout, &serr);
  int rc = vendor_populate(list, 1);
  cpm_log_end(&sout, &serr);
  SHT_ASSERT_EQ(rc, 0);

  // Test create_dir failure by placing a file where a directory should be
  sys_log("vendor", "rm -rf .cpm/vendor");
  sys_log("vendor", "touch .cpm/vendor");
  // .cpm/vendor is now a file, so vendor_populate should fail creating it!
  cpm_log_begin("vendor", &sout, &serr);
  rc = vendor_populate(list, 1);
  cpm_log_end(&sout, &serr);

  // Cleanup the poison file
  sys_log("vendor", "rm -f .cpm/vendor");

  // Restore the vendor directory which contains sht-lib for lcov source mapping
  sys_log("vendor", "../cpm vendor");

  SHT_ASSERT_EQ(rc, -1);

  vendor_teardown();
}
