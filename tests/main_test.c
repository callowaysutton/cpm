#define SHT_IMPLEMENTATION

#include "test_cache.c"
#include "test_cli.c"
#include "test_cmod.c"
#include "test_fetcher.c"
#include "test_generators.c"
#include "test_resolver.c"
#include "test_semver.c"
#include "test_vendor.c"
#include <sht.h>

int main(void) {
  sht_init_context();
  int res = sht_run_all_tests();
  sht_cleanup_context();
  return res;
}
