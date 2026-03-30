#include "../include/resolver.h"
#include "test_util.h"
#include <sht.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

SHT_SUITE(Resolver)

SHT_TEST(Resolver, test_module_node_lifecycle) {
  module_node_t node;
  module_node_init(&node);
  SHT_ASSERT_NE((void *)node.requirements, NULL);
  SHT_ASSERT_EQ(node.requirements_cap, 64);

  int rc = module_node_add_requirement(&node, "testing/path", "v1.0.0");
  SHT_ASSERT_EQ(rc, 0);
  SHT_ASSERT_EQ(node.requirements_len, 1);
  SHT_ASSERT_STR_EQ(node.requirements[0].dependent_module_path, "testing/path");
  SHT_ASSERT_STR_EQ(node.requirements[0].version, "v1.0.0");

  // trigger realloc by adding 65
  for (int i = 0; i < 65; i++) {
    rc = module_node_add_requirement(&node, "dummy", "v2.0.0");
    SHT_ASSERT_EQ(rc, 0);
  }
  SHT_ASSERT_EQ(node.requirements_len, 66);
  SHT_ASSERT_EQ(node.requirements_cap, 128);

  module_node_add_requirement(NULL, NULL, NULL); // error testing

  module_node_free(&node);
}

SHT_TEST(Resolver, test_dependency_graph_lifecycle) {
  dependency_graph_t graph;
  dependency_graph_init(&graph);
  SHT_ASSERT_NE((void *)graph.nodes, NULL);

  module_node_t *n1 = dependency_graph_add_node(&graph, "pkgA", true);
  SHT_ASSERT_NE((void *)n1, NULL);
  SHT_ASSERT_EQ(n1->is_root_dependency, true);

  module_node_t *n1_dup = dependency_graph_add_node(&graph, "pkgA", false);
  SHT_ASSERT_EQ((void *)n1, (void *)n1_dup);

  module_node_t *found = dependency_graph_find_node(&graph, "pkgA");
  SHT_ASSERT_EQ((void *)found, (void *)n1);

  module_node_t *not_found = dependency_graph_find_node(&graph, "unknown");
  SHT_ASSERT_EQ((void *)not_found, NULL);

  for (int i = 0; i < 65; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "dummy%d", i);
    dependency_graph_add_node(&graph, buf, false);
  }
  SHT_ASSERT_EQ(graph.nodes_len, 66);

  dependency_graph_add_node(NULL, NULL, false); // error testing
  dependency_graph_find_node(NULL, NULL);       // error testing

  dependency_graph_free(&graph);
}

SHT_TEST(Resolver, test_resolve_dependencies_flow) {
  sys_log("resolver", "mkdir -p .cpm/cache/dummy_cmod@v1.0.0 && mkdir -p "
         ".cpm/cache/dummy_transitive@v2.0.0");

  // Write a dummy module
  sys_log("resolver", "echo "
         "'module=\"dummy_cmod\"\\nversion=\"v1.0.0\"\\n[requires]\\ndummy_"
         "transitive = \"v2.0.0\"' > .cpm/cache/dummy_cmod@v1.0.0/cmod.toml");

  // Write a transitive module with no requires
  sys_log("resolver", "echo 'module=\"dummy_transitive\"\\nversion=\"v2.0.0\"' > "
         ".cpm/cache/dummy_transitive@v2.0.0/cmod.toml");

  cmod_t root = {0};
  root.module = "root_module";
  root.requires_len = 1;
  root.
    requires
  = calloc(1, sizeof(require_t));
  root.
    requires[
            0]
        .path = "dummy_cmod";
  root.
    requires[
            0]
        .constraint = "v1.0.0";

  resolved_t *list = NULL;
  int sout, serr;
  cpm_log_begin("resolver", &sout, &serr);
  int resolved_count = resolve_dependencies(&root, &list);
  cpm_log_end(&sout, &serr);

  // root -> dummy_cmod -> dummy_transitive
  // The list should have 2 nodes: dummy_cmod and dummy_transitive
  SHT_ASSERT_EQ(resolved_count, 2);
  SHT_ASSERT_NE((void *)list, NULL);

  // Verify properties
  int found_cmod = 0, found_transitive = 0;
  for (int i = 0; i < resolved_count; i++) {
    if (strcmp(list[i].module, "dummy_cmod") == 0)
      found_cmod = 1;
    if (strcmp(list[i].module, "dummy_transitive") == 0)
      found_transitive = 1;
  }
  SHT_ASSERT_EQ(found_cmod, 1);
  SHT_ASSERT_EQ(found_transitive, 1);

  resolved_free(list, resolved_count);
  free(root.requires);

  sys_log("resolver", "rm -rf .cpm/cache/dummy_cmod@v1.0.0");
  sys_log("resolver", "rm -rf .cpm/cache/dummy_transitive@v2.0.0");
}

SHT_TEST(Resolver, test_resolve_dependencies_errors) {
  resolve_dependencies(NULL, NULL); // error testing
  resolved_free(NULL, 0);           // error testing

  cmod_t root = {0};
  root.module = "root_err";
  root.requires_len = 1;
  root.
    requires
  = calloc(1, sizeof(require_t));
  // use a local path to avoid git network fetch
  root.
    requires[
            0]
        .path = "./non_existent";
  root.
    requires[
            0]
        .constraint = "v0.0.0";

  resolved_t *list = NULL;
  int sout, serr;
  cpm_log_begin("resolver", &sout, &serr);
  // this will attempt to fetch 'non_existent' which will fail network fetch,
  // ensuring caching fails, and skip transitive tracking, returning
  // successfully.
  int count = resolve_dependencies(&root, &list);
  cpm_log_end(&sout, &serr);
  SHT_ASSERT_EQ(count, 1); // parses what it has without transitives

  resolved_free(list, count);
  free(root.requires);
}
