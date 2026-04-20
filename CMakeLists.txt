cmake_minimum_required(VERSION 3.20)

project(WIRBLE VERSION 0.4 LANGUAGES C)

include(GNUInstallDirs)

option(INSTALL_DOCS "Install generated documentation outputs" OFF)
option(INSTALL_MANFILES "Install generated Doxygen manpages" OFF)
option(INSTALL_MANIFEST "Install WIRBLE manifests" OFF)
option(INSTALL_TOOLCHAIN "Install WIRBLE toolchain binaries" ON)
option(NO_INSTALL_MACHINES "Disable installing machine descriptions" OFF)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(wirble_third_party_dparser INTERFACE)
target_include_directories(
  wirble_third_party_dparser
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dparser
)

add_library(
  wirble_third_party_dparse
  STATIC
    third_party/dparser/arg.c
    third_party/dparser/parse.c
    third_party/dparser/scan.c
    third_party/dparser/dsymtab.c
    third_party/dparser/util.c
    third_party/dparser/read_binary.c
    third_party/dparser/dparse_tree.c
    third_party/dparser/version.c
)
target_compile_definitions(
  wirble_third_party_dparse
  PRIVATE
    D_MAJOR_VERSION=1
    D_MINOR_VERSION=30
)
target_include_directories(
  wirble_third_party_dparse
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dparser
)

add_library(
  wirble_third_party_mkdparse
  STATIC
    third_party/dparser/mkdparse.c
    third_party/dparser/write_tables.c
    third_party/dparser/grammar.g.c
    third_party/dparser/gram.c
    third_party/dparser/lex.c
    third_party/dparser/lr.c
    third_party/dparser/version.c
)
target_compile_definitions(
  wirble_third_party_mkdparse
  PRIVATE
    D_MAJOR_VERSION=1
    D_MINOR_VERSION=30
)
target_include_directories(
  wirble_third_party_mkdparse
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dparser
)
target_link_libraries(wirble_third_party_mkdparse PUBLIC wirble_third_party_dparse)

add_executable(
  wirble_make_dparser
  third_party/dparser/make_dparser.c
  third_party/dparser/version.c
)
target_compile_definitions(
  wirble_make_dparser
  PRIVATE
    D_MAJOR_VERSION=1
    D_MINOR_VERSION=30
)
target_include_directories(
  wirble_make_dparser
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dparser
)
target_link_libraries(
  wirble_make_dparser
  PRIVATE
    wirble_third_party_mkdparse
    wirble_third_party_dparse
)

add_library(
  wirble_third_party_microrl
  STATIC
    third_party/microrl/src/microrl.c
)
target_include_directories(
  wirble_third_party_microrl
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/microrl/src
)

add_library(
  wirble_third_party_sfsexp
  STATIC
    third_party/sfsexp/src/cstring.c
    third_party/sfsexp/src/event_temp.c
    third_party/sfsexp/src/faststack.c
    third_party/sfsexp/src/io.c
    third_party/sfsexp/src/parser.c
    third_party/sfsexp/src/sexp.c
    third_party/sfsexp/src/sexp_memory.c
    third_party/sfsexp/src/sexp_ops.c
    third_party/sfsexp/src/sexp_vis.c
)
target_include_directories(
  wirble_third_party_sfsexp
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sfsexp/src
)

add_library(wirble_third_party_lmdb INTERFACE)
target_include_directories(
  wirble_third_party_lmdb
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/dependencies
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/lmdb
)

set(WIRBLE_LIBRARY_SOURCES
  src/wirble/common/arena.c
  src/wirble/common/diagnostics.c
  src/wirble/common/strpool.c
  src/wirble/common/bytebuf.c
  src/wirble/wil/wil_nodes.c
  src/wirble/wil/wil_builder.c
  src/wirble/wil/wil_lower_ast.c
  src/wirble/wil/wil_validate.c
  src/wirble/wil/wil_printer.c
  src/wirble/wrs/wrs_loader.c
  src/wirble/wrs/wrs_parser.c
  src/wirble/wrs/wrs_system.c
  src/wirble/wrs/wrs_match.c
  src/wirble/wrs/wrs_apply.c
  src/wirble/wrs/wrs_debug.c
  src/wirble/mal/mal_ir.c
  src/wirble/mal/mal_builder.c
  src/wirble/mal/mal_validate.c
  src/wirble/mal/mal_printer.c
  src/wirble/mal/mrs_loader.c
  src/wirble/mal/mrs_match.c
  src/wirble/mal/mrs_apply.c
  src/wirble/mal/mrs_debug.c
  src/wirble/mds/mds_selector.c
  src/wirble/mds/mds_patterns.c
  src/wirble/mds/mds_tables.c
  src/wirble/mds/mds_printer.c
  src/wirble/tos/tos_stream.c
  src/wirble/tos/tos_opt.c
  src/wirble/tos/tos_printer.c
  src/wirble/wvm/wvm_encoder.c
  src/wirble/wvm/wvm_emit.c
  src/wirble/wvm/wvm_runtime.c
  src/wirble/wvm/wvm_printer.c
  src/wirble/support/sexp_reader.c
  src/wirble/support/hash_table.c
  src/wirble/support/filemap.c
)

add_library(wirble STATIC ${WIRBLE_LIBRARY_SOURCES})
target_compile_definitions(wirble PUBLIC WIRBLE_BUILD)
target_compile_definitions(
  wirble
  PRIVATE
    WIRBLE_STDRULE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/stdrewrite"
)
target_include_directories(
  wirble
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/dependencies
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/wirble
)
target_link_libraries(
  wirble
  PUBLIC
    wirble_third_party_dparser
    wirble_third_party_sfsexp
    wirble_third_party_lmdb
)

set(WIRBLE_EXPR_PARSER ${CMAKE_CURRENT_BINARY_DIR}/wirble_expr.g.d_parser.c)
add_custom_command(
  OUTPUT ${WIRBLE_EXPR_PARSER}
  DEPENDS wirble_make_dparser examples/common/wirble_expr.g
  COMMAND $<TARGET_FILE:wirble_make_dparser> ${CMAKE_CURRENT_SOURCE_DIR}/examples/common/wirble_expr.g -o ${WIRBLE_EXPR_PARSER}
  VERBATIM
)

set(WIRBLE_EXAMPLE_FRONTEND_SOURCES
  examples/common/frontend.c
  ${WIRBLE_EXPR_PARSER}
)

add_executable(
  wirblecc
  examples/wirblecc/main.c
  ${WIRBLE_EXAMPLE_FRONTEND_SOURCES}
)
target_compile_definitions(
  wirblecc
  PRIVATE
    WIRBLE_STDRULE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/stdrewrite"
)
target_link_libraries(
  wirblecc
  PRIVATE
    wirble
    wirble_third_party_dparse
)
target_include_directories(
  wirblecc
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(
  iwirble
  examples/iwirble/main.c
  ${WIRBLE_EXAMPLE_FRONTEND_SOURCES}
)
target_compile_definitions(
  iwirble
  PRIVATE
    WIRBLE_STDRULE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/stdrewrite"
)
target_link_libraries(
  iwirble
  PRIVATE
    wirble
    wirble_third_party_dparse
    wirble_third_party_microrl
)
target_include_directories(
  iwirble
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

foreach(
  WIRBLE_TOOL
  wil-lower
  wrs-rewrite
  mal-lower
  mal-opt
  mds-select
  tos-build
  wvm-emit
)
  add_executable(
    ${WIRBLE_TOOL}
    toolchain/${WIRBLE_TOOL}.c
    ${WIRBLE_EXAMPLE_FRONTEND_SOURCES}
  )
  target_compile_definitions(
    ${WIRBLE_TOOL}
    PRIVATE
      WIRBLE_STDRULE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/stdrewrite"
  )
  target_link_libraries(
    ${WIRBLE_TOOL}
    PRIVATE
      wirble
      wirble_third_party_dparse
  )
  target_include_directories(
    ${WIRBLE_TOOL}
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_BINARY_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/src/wirble
  )
endforeach()

add_executable(wvm-run toolchain/wvm-run.c)
target_link_libraries(wvm-run PRIVATE wirble)
target_include_directories(
  wvm-run
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/wirble
)

file(
  GENERATE
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/wirble-header-smoke.c
  CONTENT "#include <wirble/wirble-wil.h>\n#include <wirble/wirble-wrs.h>\n#include <wirble/wirble-mal.h>\n#include <wirble/wirble-mds.h>\n#include <wirble/wirble-tos.h>\n#include <wirble/wirble-wvm.h>\nint main(void) { return 0; }\n"
)

add_executable(wirble-header-smoke ${CMAKE_CURRENT_BINARY_DIR}/wirble-header-smoke.c)
target_link_libraries(wirble-header-smoke PRIVATE wirble)

file(
  GENERATE
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/common-runtime-smoke.c
  CONTENT "#include \"common/arena.h\"\n#include \"common/bytebuf.h\"\n#include \"common/diagnostics.h\"\n#include \"common/strpool.h\"\n#include <stdio.h>\n#include <string.h>\nint main(void) {\n  WirbleArena arena;\n  WirbleDiagnostics diag;\n  WirbleStrpool pool;\n  WirbleByteBuf buffer;\n  char *copy;\n  const char *lhs;\n  const char *rhs;\n  const char payload[] = \"abc\";\n  wirble_arena_init(&arena, 128u);\n  copy = wirble_arena_strdup(&arena, \"hello\");\n  if (copy == NULL || strcmp(copy, \"hello\") != 0) return 1;\n  wirble_diag_init(&diag, stderr, \"stage2\");\n  wirble_diag_reportf(&diag, WIRBLE_DIAG_WARNING, \"1:1\", \"%s\", \"warn\");\n  if (wirble_diag_count(&diag, WIRBLE_DIAG_WARNING) != 1u) return 2;\n  wirble_strpool_init(&pool);\n  lhs = wirble_strpool_intern_from_pool(&pool, \"interned\");\n  rhs = wirble_strpool_intern_from_pool(&pool, \"interned\");\n  if (lhs == NULL || lhs != rhs) return 3;\n  wirble_bytebuf_init(&buffer);\n  if (!wirble_bytebuf_append(&buffer, payload, sizeof(payload) - 1u)) return 4;\n  if (buffer.size != 3u) return 5;\n  wirble_bytebuf_destroy(&buffer);\n  wirble_strpool_destroy(&pool);\n  wirble_arena_destroy(&arena);\n  return 0;\n}\n"
)

add_executable(common-runtime-smoke ${CMAKE_CURRENT_BINARY_DIR}/common-runtime-smoke.c)
target_link_libraries(common-runtime-smoke PRIVATE wirble)
target_include_directories(common-runtime-smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/wirble)

file(
  GENERATE
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/sexp-reader-smoke.c
  CONTENT "#include \"common/diagnostics.h\"\n#include \"support/sexp_reader.h\"\n#include <stdio.h>\nint main(void) {\n  WirbleDiagnostics diag;\n  WirbleSexpTree tree;\n  WirbleSexpRuleView rule;\n  const WirbleSexpNode *root;\n  const WirbleSexpNode *first_form;\n  const WirbleSexpNode *first_match;\n  const WirbleSexpNode *quoted_form;\n  wirble_diag_init(&diag, stderr, \"stage3\");\n  wirble_sexp_tree_init(&tree, &diag);\n  if (!wirble_sexp_tree_parse_file(&tree, \"${CMAKE_CURRENT_SOURCE_DIR}/stdrewrite/arithmetic.wrs\")) return 1;\n  if (wirble_sexp_tree_form_count(&tree) != 17u) return 2;\n  root = wirble_sexp_tree_root(&tree);\n  if (root == NULL || wirble_sexp_node_child_count(root) != 17u) return 3;\n  first_form = wirble_sexp_node_child(root, 0u);\n  if (!wirble_sexp_extract_rule(first_form, &rule)) return 4;\n  if (!wirble_sexp_atom_equals(rule.name, \"add-zero-right\")) return 5;\n  first_match = wirble_sexp_node_child(rule.match, 0u);\n  if (!wirble_sexp_atom_equals(first_match, \"add\")) return 6;\n  wirble_sexp_tree_reset(&tree);\n  if (!wirble_sexp_tree_parse_cstr(&tree, \"inline\", \"(rule :name \\\"quoted\\\" :match (literal 'x) :replace \\\"done\\\")\")) return 7;\n  quoted_form = wirble_sexp_node_child(wirble_sexp_tree_root(&tree), 0u);\n  if (!wirble_sexp_extract_rule(quoted_form, &rule)) return 8;\n  if (rule.name == NULL || rule.name->atom_kind != WIRBLE_SEXP_ATOM_DOUBLE_QUOTED) return 9;\n  if (wirble_sexp_node_child(rule.match, 1u) == NULL || wirble_sexp_node_child(rule.match, 1u)->atom_kind != WIRBLE_SEXP_ATOM_SINGLE_QUOTED) return 10;\n  wirble_sexp_tree_destroy(&tree);\n  return wirble_diag_has_errors(&diag) ? 11 : 0;\n}\n"
)

add_executable(sexp-reader-smoke ${CMAKE_CURRENT_BINARY_DIR}/sexp-reader-smoke.c)
target_link_libraries(sexp-reader-smoke PRIVATE wirble)
target_include_directories(sexp-reader-smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/wirble)

file(
  GENERATE
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/wil-core-smoke.c
  CONTENT "#include <wirble/wirble-wil.h>\n#include <stdio.h>\n#include <string.h>\nint main(void) {\n  WILContext *ctx = wil_context_create();\n  WILGraph *graph;\n  WILNode *start;\n  WILNode *lhs;\n  WILNode *rhs;\n  WILNode *sum;\n  WILNode *ret;\n  FILE *tmp;\n  char buffer[2048];\n  size_t nread;\n  if (ctx == NULL) return 1;\n  graph = wil_context_graph(ctx);\n  start = wil_make_start(ctx);\n  lhs = wil_make_param(ctx, 0u, WIL_TYPE_I32, \"lhs\");\n  rhs = wil_make_param(ctx, 1u, WIL_TYPE_I32, \"rhs\");\n  sum = wil_make_add(ctx, lhs, rhs);\n  ret = wil_make_return(ctx, start, sum);\n  if (graph == NULL || wil_graph_node_count(graph) != 5u) return 2;\n  if (wil_node_input_count(sum) != 2u || wil_node_use_count(lhs) != 1u) return 3;\n  if (wil_node_input_edge_kind(ret, 0u) != WIL_EDGE_CONTROL) return 4;\n  if (!wil_verify_graph(ctx)) return 5;\n  tmp = tmpfile();\n  if (tmp == NULL) return 6;\n  wil_print_graph(graph, tmp);\n  rewind(tmp);\n  nread = fread(buffer, 1u, sizeof(buffer) - 1u, tmp);\n  fclose(tmp);\n  buffer[nread] = '\\0';\n  if (strstr(buffer, \"WIL graph: 5 nodes\") == NULL) return 7;\n  if (strstr(buffer, \"n4 = add : i32 <- data:n2 data:n3\") == NULL) return 8;\n  if (strstr(buffer, \"n5 = return : control <- control:n1 data:n4\") == NULL) return 9;\n  wil_context_destroy(ctx);\n  return 0;\n}\n"
)

add_executable(wil-core-smoke ${CMAKE_CURRENT_BINARY_DIR}/wil-core-smoke.c)
target_link_libraries(wil-core-smoke PRIVATE wirble)
target_include_directories(wil-core-smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/wirble)

file(
  GENERATE
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/wrs-engine-smoke.c
  CONTENT "#include <wirble/wirble-wil.h>\n#include <wirble/wirble-wrs.h>\nint main(void) {\n  WILContext *ctx = wil_context_create();\n  WRSFile *file;\n  WILRewriteSystem *sys;\n  WILNode *lhs;\n  WILNode *zero;\n  WILNode *add;\n  WILNode *normalized;\n  if (ctx == NULL) return 1;\n  file = wrsFileLoad(ctx, \"${CMAKE_CURRENT_SOURCE_DIR}/stdrewrite/arithmetic.wrs\");\n  if (file == NULL || wrsFileRuleCount(file) != 17u) return 2;\n  sys = wilRewriteSystemCreate(ctx);\n  if (sys == NULL) return 3;\n  wrsFileApplyToSystem(file, sys);\n  if (wilRewriteSystemRuleCount(sys) != 17u) return 4;\n  lhs = wil_make_param(ctx, 0u, WIL_TYPE_I32, \"x\");\n  zero = wil_make_const_int(ctx, 0, WIL_TYPE_I32);\n  add = wil_make_add(ctx, lhs, zero);\n  normalized = wilRewriteSystemNormalize(sys, add, 8u);\n  if (normalized != lhs) return 5;\n  if (!wilRewriteSystemIsNormalForm(sys, lhs)) return 6;\n  if (wilRewriteSystemIsNormalForm(sys, add)) return 7;\n  if (!wilRuleValidate(wrsFileGetRule(file, 0u))) return 8;\n  wil_context_destroy(ctx);\n  return 0;\n}\n"
)

add_executable(wrs-engine-smoke ${CMAKE_CURRENT_BINARY_DIR}/wrs-engine-smoke.c)
target_link_libraries(wrs-engine-smoke PRIVATE wirble)
target_include_directories(wrs-engine-smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/wirble)

file(
  GENERATE
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/stdrewrite-smoke.c
  CONTENT "#include <wirble/wirble-wil.h>\n#include <wirble/wirble-wrs.h>\nint main(void) {\n  WILContext *ctx = wil_context_create();\n  WILRewriteSystem *sys;\n  WILNode *x;\n  WILNode *y;\n  WILNode *cond;\n  WILNode *expr;\n  WILNode *normalized;\n  if (ctx == NULL) return 1;\n  if (!wrsFileValidate(ctx, \"${CMAKE_CURRENT_SOURCE_DIR}/stdrewrite/arithmetic.wrs\", NULL)) return 2;\n  if (!wrsFileValidate(ctx, \"${CMAKE_CURRENT_SOURCE_DIR}/stdrewrite/logic.wrs\", NULL)) return 3;\n  if (!wrsFileValidate(ctx, \"${CMAKE_CURRENT_SOURCE_DIR}/stdrewrite/canonical.wrs\", NULL)) return 4;\n  sys = wilRewriteSystemCreate(ctx);\n  if (sys == NULL) return 5;\n  wilRewriteSystemAddAllBuiltinRules(sys);\n  if (wilRewriteSystemRuleCount(sys) != 51u) return 6;\n  if (wilRewriteSystemFindRule(sys, \"mul-neg-one-right\") == NULL) return 7;\n  if (wilRewriteSystemFindRule(sys, \"add-self\") == NULL) return 8;\n  x = wil_make_param(ctx, 0u, WIL_TYPE_I32, \"x\");\n  y = wil_make_param(ctx, 1u, WIL_TYPE_I32, \"y\");\n  cond = wil_make_param(ctx, 2u, WIL_TYPE_I1, \"cond\");\n  expr = wil_make_mul(ctx, wil_make_add(ctx, x, wil_make_const_int(ctx, 0, WIL_TYPE_I32)), wil_make_const_int(ctx, 1, WIL_TYPE_I32));\n  normalized = wilRewriteSystemNormalize(sys, expr, 8u);\n  if (normalized != x) return 9;\n  expr = wil_make_select(ctx, wil_make_const_bool(ctx, 1), wil_make_or(ctx, cond, wil_make_const_bool(ctx, 0)), wil_make_and(ctx, cond, wil_make_const_bool(ctx, 0)));\n  normalized = wilRewriteSystemNormalize(sys, expr, 12u);\n  if (normalized != cond) return 10;\n  expr = wil_make_not(ctx, wil_make_lt(ctx, x, y));\n  normalized = wilRewriteSystemNormalize(sys, expr, 8u);\n  if (normalized == NULL || wil_node_kind(normalized) != WIL_NODE_GE) return 11;\n  if (wil_node_input(normalized, 0u) != x || wil_node_input(normalized, 1u) != y) return 12;\n  expr = wil_make_select(ctx, wil_make_not(ctx, cond), x, y);\n  normalized = wilRewriteSystemNormalize(sys, expr, 8u);\n  if (normalized == NULL || wil_node_kind(normalized) != WIL_NODE_SELECT) return 13;\n  if (wil_node_input(normalized, 0u) != cond || wil_node_input(normalized, 1u) != y || wil_node_input(normalized, 2u) != x) return 14;\n  wil_context_destroy(ctx);\n  return 0;\n}\n"
)

add_executable(stdrewrite-smoke ${CMAKE_CURRENT_BINARY_DIR}/stdrewrite-smoke.c)
target_link_libraries(stdrewrite-smoke PRIVATE wirble)
target_include_directories(stdrewrite-smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/wirble)

file(
  GENERATE
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/mal-stage-smoke.c
  CONTENT "#include <wirble/wirble-mal.h>\n#include <wirble/wirble-wil.h>\n#include \"mal/mal_ir.h\"\n#include <stdio.h>\n#include <string.h>\nint main(void) {\n  WILContext *ctx = wil_context_create();\n  MALModule *module;\n  MALFunction *fn;\n  MALBlock *block;\n  MALRewriteSystem *mrs;\n  WILNode *start;\n  WILNode *param;\n  WILNode *zero;\n  WILNode *add;\n  WILNode *ret;\n  FILE *tmp;\n  char buffer[2048];\n  size_t nread;\n  if (ctx == NULL) return 1;\n  start = wil_make_start(ctx);\n  param = wil_make_param(ctx, 0u, WIL_TYPE_I32, \"x\");\n  zero = wil_make_const_int(ctx, 0, WIL_TYPE_I32);\n  add = wil_make_add(ctx, param, zero);\n  ret = wil_make_return(ctx, start, add);\n  (void) ret;\n  module = mal_module_create(\"test-triple\");\n  if (module == NULL) return 2;\n  if (!malLowerFromWIL(module, wil_context_graph(ctx))) return 3;\n  if (module->functionCount != 1u || module->functions == NULL) return 4;\n  fn = module->functions;\n  if (fn->paramCount != 1u || fn->blocks == NULL || fn->blockCount != 1u) return 5;\n  block = fn->blocks;\n  if (block->first == NULL || block->last == NULL || block->first == block->last) return 6;\n  if (block->first->opcode != MAL_OP_ADD || block->last->opcode != MAL_OP_RET) return 7;\n  if (!mal_verify_ssa(fn)) return 8;\n  mal_compute_cfg(fn);\n  mal_compute_dominators(fn);\n  mal_compute_liveness(fn);\n  if (block->succCount != 0u || block->predCount != 0u) return 9;\n  mrs = mal_mrs_load(\"${CMAKE_CURRENT_SOURCE_DIR}/stdrewrite/mal_peephole.wrs\");\n  if (mrs == NULL || mrs->ruleCount < 6u) return 10;\n  if (!malApplyPeepholeToFixpoint(mrs, fn)) return 11;\n  if (block->first->opcode != MAL_OP_COPY) return 12;\n  tmp = tmpfile();\n  if (tmp == NULL) return 13;\n  mal_dump_function(fn);\n  mal_dump_module(module);\n  mal_print_function_to_file(tmp, fn);\n  rewind(tmp);\n  nread = fread(buffer, 1u, sizeof(buffer) - 1u, tmp);\n  fclose(tmp);\n  buffer[nread] = '\\0';\n  if (strstr(buffer, \"function wil_main -> i32\") == NULL) return 14;\n  if (strstr(buffer, \"copy.i32 r0\") == NULL) return 15;\n  mal_mrs_destroy(mrs);\n  mal_module_destroy(module);\n  wil_context_destroy(ctx);\n  return 0;\n}\n"
)

add_executable(mal-stage-smoke ${CMAKE_CURRENT_BINARY_DIR}/mal-stage-smoke.c)
target_link_libraries(mal-stage-smoke PRIVATE wirble)
target_include_directories(mal-stage-smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/wirble)

file(
  GENERATE
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/mds-stage-smoke.c
  CONTENT "#include <wirble/wirble-mal.h>\n#include <wirble/wirble-mds.h>\n#include <wirble/wirble-wil.h>\n#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\nint main(void) {\n  WILContext *ctx = wil_context_create();\n  MALModule *module;\n  MALFunction *fn;\n  MALBlock *block;\n  MALRewriteSystem *mrs;\n  MDSMachine *machine;\n  MDSInstrSelector *selector;\n  MDSProgram *program;\n  unsigned char *buffer;\n  size_t bufferSize = 0u;\n  FILE *in;\n  char text[1024];\n  size_t nread;\n  WILNode *start;\n  WILNode *param;\n  WILNode *zero;\n  WILNode *add;\n  WILNode *ret;\n  if (ctx == NULL) return 1;\n  start = wil_make_start(ctx);\n  param = wil_make_param(ctx, 0u, WIL_TYPE_I32, \"x\");\n  zero = wil_make_const_int(ctx, 0, WIL_TYPE_I32);\n  add = wil_make_add(ctx, param, zero);\n  ret = wil_make_return(ctx, start, add);\n  (void) ret;\n  module = mal_module_create(\"x86_64-unknown-none\");\n  if (module == NULL || !malLowerFromWIL(module, wil_context_graph(ctx))) return 2;\n  fn = module->functions;\n  block = fn == NULL ? NULL : fn->blocks;\n  if (fn == NULL || block == NULL || block->first == NULL) return 3;\n  mrs = mal_mrs_load(\"${CMAKE_CURRENT_SOURCE_DIR}/stdrewrite/mal_peephole.wrs\");\n  if (mrs == NULL || !malApplyPeepholeToFixpoint(mrs, fn)) return 4;\n  if (block->first->opcode != MAL_OP_COPY) return 5;\n  machine = mdsLoadMachine(\"${CMAKE_CURRENT_SOURCE_DIR}/machines/x86_64.json\", MDS_FORMAT_JSON);\n  if (machine == NULL || !mdsValidateMachine(machine)) return 6;\n  selector = mdsCreateSelector(machine);\n  if (selector == NULL) return 7;\n  program = malLowerToMDS(module, selector);\n  if (program == NULL || program->blockCount != 1u) return 8;\n  if (program->blocks[0].instrCount != 2u) return 9;\n  if (strcmp(program->blocks[0].instructions[0].desc->mnemonic, \"x64.mov.rr\") != 0) return 10;\n  if (strcmp(program->blocks[0].instructions[1].desc->mnemonic, \"x64.ret\") != 0) return 11;\n  buffer = mdsEmitBuffer(program, &bufferSize);\n  if (buffer == NULL || bufferSize == 0u) return 12;\n  free(buffer);\n  if (!mdsEmitAssembly(program, \"${CMAKE_CURRENT_BINARY_DIR}/mds-stage-smoke.out\")) return 13;\n  in = fopen(\"${CMAKE_CURRENT_BINARY_DIR}/mds-stage-smoke.out\", \"r\");\n  if (in == NULL) return 14;\n  nread = fread(text, 1u, sizeof(text) - 1u, in);\n  fclose(in);\n  text[nread] = '\\0';\n  if (strstr(text, \"x64.mov.rr r1, r0\") == NULL) return 15;\n  if (strstr(text, \"x64.ret r1\") == NULL) return 16;\n  mdsDestroyProgram(program);\n  mdsDestroySelector(selector);\n  mdsDestroyMachine(machine);\n  mal_mrs_destroy(mrs);\n  mal_module_destroy(module);\n  wil_context_destroy(ctx);\n  return 0;\n}\n"
)

add_executable(mds-stage-smoke ${CMAKE_CURRENT_BINARY_DIR}/mds-stage-smoke.c)
target_link_libraries(mds-stage-smoke PRIVATE wirble)
target_include_directories(mds-stage-smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/wirble)

file(
  GENERATE
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tos-stage-smoke.c
  CONTENT "#include <wirble/wirble-mal.h>\n#include <wirble/wirble-mds.h>\n#include <wirble/wirble-tos.h>\n#include <wirble/wirble-wil.h>\n#include <stdio.h>\n#include <string.h>\nint main(void) {\n  WILContext *ctx = wil_context_create();\n  MALModule *module;\n  MALFunction *fn;\n  MALRewriteSystem *mrs;\n  MDSMachine *machine;\n  MDSInstrSelector *selector;\n  MDSProgram *mdsProgram;\n  TOSProgram *tosProgram;\n  TOSContext *tosCtx;\n  FILE *in;\n  char text[1024];\n  size_t nread;\n  WILNode *start;\n  WILNode *param;\n  WILNode *zero;\n  WILNode *add;\n  WILNode *ret;\n  if (ctx == NULL) return 1;\n  start = wil_make_start(ctx);\n  param = wil_make_param(ctx, 0u, WIL_TYPE_I32, \"x\");\n  zero = wil_make_const_int(ctx, 0, WIL_TYPE_I32);\n  add = wil_make_add(ctx, param, zero);\n  ret = wil_make_return(ctx, start, add);\n  (void) ret;\n  module = mal_module_create(\"x86_64-unknown-none\");\n  if (module == NULL || !malLowerFromWIL(module, wil_context_graph(ctx))) return 2;\n  fn = module->functions;\n  if (fn == NULL) return 3;\n  mrs = mal_mrs_load(\"${CMAKE_CURRENT_SOURCE_DIR}/stdrewrite/mal_peephole.wrs\");\n  if (mrs == NULL || !malApplyPeepholeToFixpoint(mrs, fn)) return 4;\n  machine = mdsLoadMachine(\"${CMAKE_CURRENT_SOURCE_DIR}/machines/x86_64.json\", MDS_FORMAT_JSON);\n  if (machine == NULL || !mdsValidateMachine(machine)) return 5;\n  selector = mdsCreateSelector(machine);\n  if (selector == NULL) return 6;\n  mdsProgram = malLowerToMDS(module, selector);\n  if (mdsProgram == NULL) return 7;\n  tosProgram = tosBuildFromMDS(mdsProgram);\n  if (tosProgram == NULL || tosProgram->blockCount != 1u || tosProgram->instrCount != 2u) return 8;\n  if (tosProgram->orderKind != TOS_ORDER_INPUT || tosProgram->isFinalized) return 9;\n  tosCtx = tos_create(NULL, machine);\n  if (tosCtx == NULL || !tos_optimize_program(tosCtx, tosProgram)) return 10;\n  if (!tosValidateProgram(tosProgram)) return 11;\n  if (tosProgram->orderKind != TOS_ORDER_REVERSE_POSTORDER || !tosProgram->isFinalized) return 12;\n  if (!tosEmitText(tosProgram, \"${CMAKE_CURRENT_BINARY_DIR}/tos-stage-smoke.out\")) return 13;\n  in = fopen(\"${CMAKE_CURRENT_BINARY_DIR}/tos-stage-smoke.out\", \"r\");\n  if (in == NULL) return 14;\n  nread = fread(text, 1u, sizeof(text) - 1u, in);\n  fclose(in);\n  text[nread] = '\\0';\n  if (strstr(text, \"tos-program x86_64\") == NULL) return 15;\n  if (strstr(text, \"layout b0\") == NULL) return 16;\n  if (strstr(text, \"t0: x64.mov.rr r1, r0\") == NULL) return 17;\n  if (strstr(text, \"t1: x64.ret r1\") == NULL) return 18;\n  tos_destroy(tosCtx);\n  tosDestroyProgram(tosProgram);\n  mdsDestroyProgram(mdsProgram);\n  mdsDestroySelector(selector);\n  mdsDestroyMachine(machine);\n  mal_mrs_destroy(mrs);\n  mal_module_destroy(module);\n  wil_context_destroy(ctx);\n  return 0;\n}\n"
)

add_executable(tos-stage-smoke ${CMAKE_CURRENT_BINARY_DIR}/tos-stage-smoke.c)
target_link_libraries(tos-stage-smoke PRIVATE wirble)
target_include_directories(tos-stage-smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/wirble)

file(
  GENERATE
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/wvm-stage-smoke.c
  CONTENT "#include <wirble/wirble-mal.h>\n#include <wirble/wirble-mds.h>\n#include <wirble/wirble-tos.h>\n#include <wirble/wirble-wil.h>\n#include <wirble/wirble-wvm.h>\n#include \"wvm/wvm_encoder.h\"\n#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\nint main(void) {\n  WILContext *ctx = wil_context_create();\n  MALModule *module;\n  MALFunction *fn;\n  MALRewriteSystem *mrs;\n  MDSMachine *machine;\n  MDSInstrSelector *selector;\n  MDSProgram *mdsProgram;\n  TOSProgram *tosProgram;\n  TOSContext *tosCtx;\n  WVMPrototype *proto;\n  WVMPrototype *roundTrip;\n  WVMState *vm;\n  WVMBuffer buf;\n  WVMReader reader;\n  char *disasm;\n  WILNode *start;\n  WILNode *lhs;\n  WILNode *rhs;\n  WILNode *add;\n  WILNode *ret;\n  int rc = 1;\n  if (ctx == NULL) return 1;\n  start = wil_make_start(ctx);\n  lhs = wil_make_param(ctx, 0u, WIL_TYPE_I32, \"lhs\");\n  rhs = wil_make_param(ctx, 1u, WIL_TYPE_I32, \"rhs\");\n  add = wil_make_add(ctx, lhs, rhs);\n  ret = wil_make_return(ctx, start, add);\n  (void) ret;\n  module = mal_module_create(\"x86_64-unknown-none\");\n  if (module == NULL || !malLowerFromWIL(module, wil_context_graph(ctx))) return 2;\n  fn = module->functions;\n  if (fn == NULL) return 3;\n  mrs = mal_mrs_load(\"${CMAKE_CURRENT_SOURCE_DIR}/stdrewrite/mal_peephole.wrs\");\n  if (mrs == NULL) return 4;\n  (void) malApplyPeepholeToFixpoint(mrs, fn);\n  machine = mdsLoadMachine(\"${CMAKE_CURRENT_SOURCE_DIR}/machines/x86_64.json\", MDS_FORMAT_JSON);\n  if (machine == NULL || !mdsValidateMachine(machine)) return 5;\n  selector = mdsCreateSelector(machine);\n  if (selector == NULL) return 6;\n  mdsProgram = malLowerToMDS(module, selector);\n  if (mdsProgram == NULL) return 7;\n  tosProgram = tosBuildFromMDS(mdsProgram);\n  if (tosProgram == NULL) return 8;\n  tosCtx = tos_create(NULL, machine);\n  if (tosCtx == NULL || !tos_optimize_program(tosCtx, tosProgram)) return 9;\n  proto = wvmPrototypeFromTOS(tosProgram, \"stage10\", \"<smoke>\");\n  if (proto == NULL || !wvmVerifyPrototype(NULL, proto)) return 10;\n  buf = wvmBufferCreate(NULL, 128u);\n  if (!wvmSerializePrototype(proto, &buf)) return 11;\n  reader = wvmReaderCreate(buf.data, buf.len);\n  roundTrip = wvmDeserializePrototype(NULL, &reader);\n  if (roundTrip == NULL || !wvmVerifyPrototype(NULL, roundTrip)) return 12;\n  if (roundTrip->instrCount != proto->instrCount || roundTrip->constCount != proto->constCount) return 13;\n  vm = wvmCreate();\n  if (vm == NULL) return 14;\n  wvmSetReg(vm, 0u, WVM_VALUE_FROM_INT(7));\n  wvmSetReg(vm, 1u, WVM_VALUE_FROM_INT(5));\n  if (!wvmExecute(vm, roundTrip)) return 15;\n  if (!WVM_IS_INT(wvmGetReg(vm, 0u)) || WVM_AS_INT(wvmGetReg(vm, 0u)) != 12) return 16;\n  disasm = wvmDisassemble(vm, roundTrip);\n  if (disasm == NULL) return 17;\n  if (strstr(disasm, \"add r2, r0, r1\") == NULL) return 18;\n  if (strstr(disasm, \"return r2\") == NULL) return 19;\n  free(disasm);\n  rc = 0;\n  wvmDestroy(vm);\n  wvmPrototypeDestroyInternal(NULL, roundTrip);\n  wvmBufferDestroy(NULL, &buf);\n  wvmPrototypeDestroyInternal(NULL, proto);\n  tos_destroy(tosCtx);\n  tosDestroyProgram(tosProgram);\n  mdsDestroyProgram(mdsProgram);\n  mdsDestroySelector(selector);\n  mdsDestroyMachine(machine);\n  mal_mrs_destroy(mrs);\n  mal_module_destroy(module);\n  wil_context_destroy(ctx);\n  return rc;\n}\n"
)

add_executable(wvm-stage-smoke ${CMAKE_CURRENT_BINARY_DIR}/wvm-stage-smoke.c)
target_link_libraries(wvm-stage-smoke PRIVATE wirble)
target_include_directories(wvm-stage-smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/wirble)

file(
  GENERATE
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/stage12-opt-validation-smoke.c
  CONTENT "#include \"examples/common/frontend.h\"\n#include <wirble/wirble-wvm.h>\n#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\nint main(void) {\n  static const char *source = \"((8 - 3) * (2 + 4)) / 5;\";\n  WirbleCompileOptions opt0;\n  WirbleCompileOptions opt2;\n  WVMPrototype *proto0;\n  WVMPrototype *proto2;\n  WVMValue value0;\n  WVMValue value2;\n  char *errorMessage = NULL;\n  memset(&opt0, 0, sizeof(opt0));\n  memset(&opt2, 0, sizeof(opt2));\n  opt0.target = \"x86_64\";\n  opt0.optimizationLevel = 0u;\n  opt0.verifyIR = 1;\n  opt2.target = \"x86_64\";\n  opt2.optimizationLevel = 2u;\n  opt2.verifyIR = 1;\n  proto0 = wirbleCompileSourceToPrototype(source, \"<stage12-o0>\", &opt0, &errorMessage);\n  if (proto0 == NULL) return 1;\n  if (!wirbleExecuteSource(source, \"<stage12-o0>\", &opt0, &value0, &errorMessage)) return 2;\n  if (!WVM_IS_INT(value0) || WVM_AS_INT(value0) != 6) return 3;\n  proto2 = wirbleCompileSourceToPrototype(source, \"<stage12-o2>\", &opt2, &errorMessage);\n  if (proto2 == NULL) return 4;\n  if (!wirbleExecuteSource(source, \"<stage12-o2>\", &opt2, &value2, &errorMessage)) return 5;\n  if (!WVM_IS_INT(value2) || WVM_AS_INT(value2) != 6) return 6;\n  if (!wvmVerifyPrototype(NULL, proto0) || !wvmVerifyPrototype(NULL, proto2)) return 7;\n  if (proto0->instrCount < 2u || proto2->instrCount < 2u) return 8;\n  if (proto2->instrCount > proto0->instrCount) return 9;\n  if (proto2->constCount > proto0->constCount) return 10;\n  wvmFreePrototype(NULL, proto2);\n  wvmFreePrototype(NULL, proto0);\n  wirbleFreeError(errorMessage);\n  return 0;\n}\n"
)

add_executable(
  stage12-opt-validation-smoke
  ${CMAKE_CURRENT_BINARY_DIR}/stage12-opt-validation-smoke.c
  ${WIRBLE_EXAMPLE_FRONTEND_SOURCES}
)
target_compile_definitions(
  stage12-opt-validation-smoke
  PRIVATE
    WIRBLE_STDRULE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/stdrewrite"
)
target_link_libraries(
  stage12-opt-validation-smoke
  PRIVATE
    wirble
    wirble_third_party_dparse
)
target_include_directories(
  stage12-opt-validation-smoke
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

file(
  GENERATE
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/stage12-mrs-smoke.c
  CONTENT "#include <wirble/wirble-mal.h>\n#include \"mal/mal_ir.h\"\nint main(void) {\n  MALModule *module = mal_module_create(\"x86_64\");\n  MALFunction *fn;\n  MALBlock *block;\n  MALRewriteSystem *mrs;\n  MALInst *inst;\n  if (module == NULL) return 1;\n  fn = mal_function_create(module, \"peephole\", MAL_TYPE_I32);\n  if (fn == NULL) return 2;\n  block = mal_block_create(fn, \"entry\");\n  if (block == NULL) return 3;\n  inst = mal_inst_create(MAL_OP_MUL, MAL_TYPE_I32, 0u);\n  if (inst == NULL) return 4;\n  mal_inst_add_operand(inst, mal_operand_reg(1u, MAL_TYPE_I32));\n  mal_inst_add_operand(inst, mal_operand_imm_int(0, MAL_TYPE_I32));\n  mal_block_insert_end(block, inst);\n  inst = mal_inst_create(MAL_OP_RET, MAL_TYPE_I32, MAL_INVALID_REG);\n  if (inst == NULL) return 5;\n  mal_inst_add_operand(inst, mal_operand_reg(0u, MAL_TYPE_I32));\n  mal_block_insert_end(block, inst);\n  mrs = mal_mrs_load(NULL);\n  if (mrs == NULL || mrs->ruleCount < 10u) return 6;\n  if (!malApplyPeepholeToFixpoint(mrs, fn)) return 7;\n  if (block->first == NULL || block->first->opcode != MAL_OP_COPY) return 8;\n  if (block->first->operandCount != 1u) return 9;\n  if (block->first->operands[0].kind != MAL_OPND_IMM_INT || block->first->operands[0].imm_i != 0) return 10;\n  mal_mrs_destroy(mrs);\n  mal_module_destroy(module);\n  return 0;\n}\n"
)

add_executable(stage12-mrs-smoke ${CMAKE_CURRENT_BINARY_DIR}/stage12-mrs-smoke.c)
target_link_libraries(stage12-mrs-smoke PRIVATE wirble)
target_include_directories(stage12-mrs-smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/wirble)

file(
  GENERATE
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/toolchain-cli-smoke.sh
  CONTENT "#!/usr/bin/env sh\nset -eu\n\"$1\" --dump \"${CMAKE_CURRENT_SOURCE_DIR}/examples/stage11.expr\" >/dev/null\n\"$2\" --rules \"${CMAKE_CURRENT_SOURCE_DIR}/stdrewrite/arithmetic.wrs\" \"${CMAKE_CURRENT_SOURCE_DIR}/examples/stage11.expr\" >/dev/null\n\"$3\" --dump \"${CMAKE_CURRENT_SOURCE_DIR}/examples/stage11.expr\" >/dev/null\n\"$4\" --stats \"${CMAKE_CURRENT_SOURCE_DIR}/examples/stage11.expr\" >/dev/null\n\"$5\" --dump \"${CMAKE_CURRENT_SOURCE_DIR}/examples/stage11.expr\" >/dev/null\n\"$6\" --optimize \"${CMAKE_CURRENT_SOURCE_DIR}/examples/stage11.expr\" >/dev/null\n\"$7\" --output \"${CMAKE_CURRENT_BINARY_DIR}/toolchain-smoke.wvmo\" \"${CMAKE_CURRENT_SOURCE_DIR}/examples/stage11.expr\" >/dev/null\n\"$8\" \"${CMAKE_CURRENT_BINARY_DIR}/toolchain-smoke.wvmo\" >/dev/null\n"
)

include(CTest)
if(BUILD_TESTING)
  add_test(NAME wirblecc-bootstrap COMMAND wirblecc ${CMAKE_CURRENT_SOURCE_DIR}/examples/stage11.expr -o ${CMAKE_CURRENT_BINARY_DIR}/stage11.wvmo)
  add_test(NAME wirblecc-bootstrap-o0 COMMAND wirblecc -O0 ${CMAKE_CURRENT_SOURCE_DIR}/examples/stage11.expr -o ${CMAKE_CURRENT_BINARY_DIR}/stage11-o0.wvmo)
  add_test(NAME iwirble-bootstrap COMMAND iwirble ${CMAKE_CURRENT_SOURCE_DIR}/examples/stage11.expr)
  add_test(NAME iwirble-bootstrap-o0 COMMAND iwirble -O0 ${CMAKE_CURRENT_SOURCE_DIR}/examples/stage11.expr)
  add_test(NAME public-header-smoke COMMAND wirble-header-smoke)
  add_test(NAME common-runtime-smoke COMMAND common-runtime-smoke)
  add_test(NAME sexp-reader-smoke COMMAND sexp-reader-smoke)
  add_test(NAME wil-core-smoke COMMAND wil-core-smoke)
  add_test(NAME wrs-engine-smoke COMMAND wrs-engine-smoke)
  add_test(NAME stdrewrite-smoke COMMAND stdrewrite-smoke)
  add_test(NAME mal-stage-smoke COMMAND mal-stage-smoke)
  add_test(NAME mds-stage-smoke COMMAND mds-stage-smoke)
  add_test(NAME tos-stage-smoke COMMAND tos-stage-smoke)
  add_test(NAME wvm-stage-smoke COMMAND wvm-stage-smoke)
  add_test(NAME stage12-opt-validation-smoke COMMAND stage12-opt-validation-smoke)
  add_test(NAME stage12-mrs-smoke COMMAND stage12-mrs-smoke)
  add_test(
    NAME toolchain-cli-smoke
    COMMAND sh ${CMAKE_CURRENT_BINARY_DIR}/toolchain-cli-smoke.sh
            $<TARGET_FILE:wil-lower>
            $<TARGET_FILE:wrs-rewrite>
            $<TARGET_FILE:mal-lower>
            $<TARGET_FILE:mal-opt>
            $<TARGET_FILE:mds-select>
            $<TARGET_FILE:tos-build>
            $<TARGET_FILE:wvm-emit>
            $<TARGET_FILE:wvm-run>
  )
endif()

add_subdirectory(dependencies)
add_subdirectory(stdrewrite)
add_subdirectory(manifests)
add_subdirectory(machines)
add_subdirectory(toolchain)
add_subdirectory(docs)
