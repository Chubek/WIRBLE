/**
 * @file Doxygen.i
 * @brief Main Doxygen documentation file for the Equinox E-Graph Library
 */

/**
 * @mainpage Equinox E-Graph Library
 *
 * @section intro_sec Introduction
 *
 * Equinox is a high-performance C library for equality saturation and term rewriting
 * using e-graphs (equality graphs). E-graphs are data structures that efficiently
 * represent equivalence classes of terms and enable powerful program optimization
 * and theorem proving techniques.
 *
 * @section features_sec Key Features
 *
 * - **Efficient E-Graph Implementation**: Union-find based equivalence tracking with
 *   path compression and union by rank
 * - **Hash Consing**: Structural sharing of terms for memory efficiency
 * - **Congruence Closure**: Automatic propagation of equalities through term structure
 * - **Pattern Matching**: Flexible pattern language with variables and applications
 * - **Rewrite Rules**: Declarative term rewriting with optional conditions
 * - **Equality Saturation**: Apply rewrite rules exhaustively to discover optimizations
 * - **Memory Management**: Integration with TLSF allocator for predictable performance
 * - **Utility Library**: Hash functions, string pools, and I/O utilities
 *
 * @section arch_sec Architecture
 *
 * The library is organized into several key components:
 *
 * - **E-Node** (@ref enode.h): Represents terms with operators and children
 * - **E-Class** (@ref eclass.h): Equivalence classes containing equivalent e-nodes
 * - **E-Graph** (@ref egraph.h): Main data structure orchestrating all components
 * - **Union-Find** (@ref unionfind.h): Efficient equivalence relation tracking
 * - **Hash Consing** (@ref hashcons.h): Structural sharing via hash table
 * - **Rewrite System** (@ref rewrite.h): Pattern matching and term rewriting
 * - **Utilities** (@ref libutils.h): Memory, hashing, and string utilities
 *
 * @section usage_sec Basic Usage
 *
 * @subsection usage_create Creating an E-Graph
 *
 * @code{.c}
 * #include "equinox/equinox.h"
 *
 * // Create e-graph with default configuration
 * eqx_egraph_config_t config = eqx_egraph_config_default();
 * eqx_egraph_t *egraph = eqx_egraph_create(&config);
 * @endcode
 *
 * @subsection usage_add Adding Terms
 *
 * @code{.c}
 * // Add constant: a
 * eqx_eclass_id_t a = eqx_egraph_add(egraph, OP_CONST_A, NULL, 0);
 *
 * // Add constant: b
 * eqx_eclass_id_t b = eqx_egraph_add(egraph, OP_CONST_B, NULL, 0);
 *
 * // Add application: f(a, b)
 * eqx_eclass_id_t children[] = {a, b};
 * eqx_eclass_id_t f_ab = eqx_egraph_add(egraph, OP_F, children, 2);
 * @endcode
 *
 * @subsection usage_union Asserting Equality
 *
 * @code{.c}
 * // Assert that a and b are equal
 * eqx_egraph_union(egraph, a, b);
 *
 * // Rebuild to propagate equalities
 * eqx_egraph_rebuild(egraph);
 * @endcode
 *
 * @subsection usage_rewrite Rewriting Terms
 *
 * @code{.c}
 * // Create pattern: f(x, x) -> g(x)
 * eqx_pattern_t *x1 = eqx_pattern_var("x");
 * eqx_pattern_t *x2 = eqx_pattern_var("x");
 * eqx_pattern_t *lhs_children[] = {x1, x2};
 * eqx_pattern_t *lhs = eqx_pattern_app(OP_F, lhs_children, 2);
 *
 * eqx_pattern_t *x3 = eqx_pattern_var("x");
 * eqx_pattern_t *rhs_children[] = {x3};
 * eqx_pattern_t *rhs = eqx_pattern_app(OP_G, rhs_children, 1);
 *
 * // Create rewrite rule
 * eqx_rewrite_rule_t *rule = eqx_rewrite_rule_create("simplify", lhs, rhs, NULL);
 *
 * // Apply rule to all e-classes
 * eqx_rewrite_rule_t *rules[] = {rule};
 * size_t applied = eqx_rewrite_apply_all(egraph, rules, 1, 100);
 *
 * // Clean up
 * eqx_rewrite_rule_destroy(rule);
 * @endcode
 *
 * @subsection usage_cleanup Cleanup
 *
 * @code{.c}
 * eqx_egraph_destroy(egraph);
 * @endcode
 *
 * @section examples_sec Examples
 *
 * @subsection ex_arithmetic Arithmetic Simplification
 *
 * @code{.c}
 * // Create e-graph
 * eqx_egraph_config_t config = eqx_egraph_config_default();
 * eqx_egraph_t *eg = eqx_egraph_create(&config);
 *
 * // Add terms: x + 0
 * eqx_eclass_id_t x = eqx_egraph_add(eg, OP_VAR_X, NULL, 0);
 * eqx_eclass_id_t zero = eqx_egraph_add(eg, OP_CONST_0, NULL, 0);
 * eqx_eclass_id_t children[] = {x, zero};
 * eqx_eclass_id_t x_plus_0 = eqx_egraph_add(eg, OP_ADD, children, 2);
 *
 * // Create rewrite rule: x + 0 -> x
 * eqx_pattern_t *pat_x = eqx_pattern_var("x");
 * eqx_pattern_t *pat_0 = eqx_pattern_app(OP_CONST_0, NULL, 0);
 * eqx_pattern_t *lhs_ch[] = {pat_x, pat_0};
 * eqx_pattern_t *lhs = eqx_pattern_app(OP_ADD, lhs_ch, 2);
 * eqx_pattern_t *rhs = eqx_pattern_var("x");
 *
 * eqx_rewrite_rule_t *rule = eqx_rewrite_rule_create("add_zero", lhs, rhs, NULL);
 *
 * // Apply rule
 * eqx_rewrite_apply(rule, eg, x_plus_0);
 * eqx_egraph_rebuild(eg);
 *
 * // Now x and (x + 0) are in the same e-class
 * assert(eqx_egraph_find(eg, x) == eqx_egraph_find(eg, x_plus_0));
 *
 * // Cleanup
 * eqx_rewrite_rule_destroy(rule);
 * eqx_egraph_destroy(eg);
 * @endcode
 *
 * @subsection ex_commutativity Commutativity
 *
 * @code{.c}
 * // Create rule: x + y -> y + x
 * eqx_pattern_t *x1 = eqx_pattern_var("x");
 * eqx_pattern_t *y1 = eqx_pattern_var("y");
 * eqx_pattern_t *lhs_ch[] = {x1, y1};
 * eqx_pattern_t *lhs = eqx_pattern_app(OP_ADD, lhs_ch, 2);
 *
 * eqx_pattern_t *y2 = eqx_pattern_var("y");
 * eqx_pattern_t *x2 = eqx_pattern_var("x");
 * eqx_pattern_t *rhs_ch[] = {y2, x2};
 * eqx_pattern_t *rhs = eqx_pattern_app(OP_ADD, rhs_ch, 2);
 *
 * eqx_rewrite_rule_t *comm = eqx_rewrite_rule_create("commutative", lhs, rhs, NULL);
 * @endcode
 *
 * @section perf_sec Performance Considerations
 *
 * - **Hash Consing**: Ensures each unique term is stored only once
 * - **Union-Find**: Near-constant time equivalence queries with path compression
 * - **Congruence Closure**: Efficient propagation using parent tracking
 * - **Memory Pooling**: TLSF allocator provides O(1) allocation/deallocation
 * - **Iteration Limits**: Configurable bounds prevent infinite rewriting loops
 *
 * @section config_sec Configuration
 *
 * The e-graph behavior can be customized via @ref eqx_egraph_config_t:
 *
 * - `initial_capacity`: Initial number of e-classes (default: 1024)
 * - `hashcons_capacity`: Hash table size (default: 2048)
 * - `hashcons_load_factor`: Resize threshold (default: 0.75)
 * - `rebuild_limit`: Max congruence closure iterations (default: 100)
 *
 * @section thread_sec Thread Safety
 *
 * The library is **not thread-safe** by default. Each e-graph instance should be
 * accessed by a single thread, or external synchronization must be provided.
 *
 * @section memory_sec Memory Management
 *
 * - All `create` functions return dynamically allocated objects
 * - All objects must be freed with corresponding `destroy` functions
 * - The library uses standard `malloc`/`free` internally
 * - Integration with TLSF allocator available via libutils
 *
 * @section license_sec License
 *
 * Equinox E-Graph Library
 * Copyright (c) 2025
 *
 * Licensed under the MIT License.
 *
 * @section refs_sec References
 *
 * - Willsey et al. "egg: Fast and Extensible Equality Saturation" (POPL 2021)
 * - Nelson & Oppen "Fast Decision Procedures Based on Congruence Closure" (JACM 1980)
 * - Tarjan "Efficiency of a Good But Not Linear Set Union Algorithm" (JACM 1975)
 *
 * @section contact_sec Contact
 *
 * For bug reports and feature requests, please visit the project repository.
 */

/**
 * @defgroup core Core Components
 * @brief Core e-graph data structures and algorithms
 */

/**
 * @defgroup rewrite Rewrite System
 * @brief Pattern matching and term rewriting
 */

/**
 * @defgroup utils Utilities
 * @brief Memory management, hashing, and helper functions
 */

/**
 * @page building Building the Library
 *
 * @section build_cmake CMake Build
 *
 * @code{.sh}
 * mkdir build
 * cd build
 * cmake ..
 * cmake --build .
 * @endcode
 *
 * @section build_options Build Options
 *
 * - `BUILD_TESTS`: Build test suite (default: ON)
 * - `BUILD_DOCS`: Build documentation (default: OFF)
 * - `BUILD_EXAMPLES`: Build example programs (default: ON)
 * - `ENABLE_ASAN`: Enable AddressSanitizer (default: OFF)
 * - `ENABLE_UBSAN`: Enable UndefinedBehaviorSanitizer (default: OFF)
 *
 * @section build_install Installation
 *
 * @code{.sh}
 * cmake --install .
 * @endcode
 *
 * @section build_link Linking
 *
 * @code{.cmake}
 * find_package(Equinox REQUIRED)
 * target_link_libraries(your_target PRIVATE Equinox::equinox)
 * @endcode
 */

/**
 * @page testing Testing
 *
 * @section test_run Running Tests
 *
 * @code{.sh}
 * cd build
 * ctest --output-on-failure
 * @endcode
 *
 * @section test_coverage Coverage
 *
 * @code{.sh}
 * cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON ..
 * cmake --build .
 * ctest
 * gcovr -r .. --html --html-details -o coverage.html
 * @endcode
 *
 * @section test_sanitizers Sanitizers
 *
 * @code{.sh}
 * cmake -DENABLE_ASAN=ON -DENABLE_UBSAN=ON ..
 * cmake --build .
 * ctest
 * @endcode
 */

/**
 * @page advanced Advanced Topics
 *
 * @section adv_custom Custom Operators
 *
 * Operators are represented as integers (`eqx_operator_t`). You can define
 * your own operator constants:
 *
 * @code{.c}
 * typedef enum {
 *     OP_ADD = 1,
 *     OP_MUL = 2,
 *     OP_NEG = 3,
 *     OP_CONST = 100,
 *     // ... more operators
 * } my_operators_t;
 * @endcode
 *
 * @section adv_conditions Conditional Rewriting
 *
 * Rewrite rules can have conditions that must be satisfied:
 *
 * @code{.c}
 * bool is_positive(const eqx_subst_t *subst, void *ctx) {
 *     eqx_eclass_id_t x_id;
 *     if (!eqx_subst_lookup(subst, "x", &x_id)) {
 *         return false;
 *     }
 *     // Check if x represents a positive constant
 *     // (requires custom analysis)
 *     return check_positive(x_id, ctx);
 * }
 *
 * eqx_rewrite_rule_t *rule = eqx_rewrite_rule_create(
 *     "sqrt_positive", lhs, rhs, is_positive
 * );
 * @endcode
 *
 * @section adv_extraction Term Extraction
 *
 * After equality saturation, you typically want to extract the "best" term
 * from an e-class. This requires implementing a cost function and extraction
 * algorithm (not included in core library):
 *
 * @code{.c}
 * // Pseudo-code for extraction
 * typedef size_t (*cost_fn_t)(eqx_enode_t *node);
 *
 * eqx_enode_t* extract_best(eqx_egraph_t *eg, eqx_eclass_id_t id, cost_fn_t cost) {
 *     eqx_eclass_t *ec = eqx_egraph_get_class(eg, id);
 *     eqx_enode_t *best = NULL;
 *     size_t min_cost = SIZE_MAX;
 *     
 *     for (eqx_enode_t *node = ec->nodes; node; node = node->next) {
 *         size_t c = cost(node);
 *         if (c < min_cost) {
 *             min_cost = c;
 *             best = node;
 *         }
 *     }
 *     return best;
 * }
 * @endcode
 *
 * @section adv_analysis E-Class Analysis
 *
 * You can attach custom data to e-classes for analysis:
 *
 * @code{.c}
 * // Extend e-class with custom data
 * typedef struct {
 *     eqx_eclass_t base;
 *     int constant_value;  // For constant folding
 *     bool is_constant;
 * } analyzed_eclass_t;
 * @endcode
 *
 * @section adv_persistence Persistent E-Graphs
 *
 * For persistent storage, serialize the e-graph structure:
 *
 * @code{.c}
 * // Pseudo-code for serialization
 * void serialize_egraph(eqx_egraph_t *eg, FILE *fp) {
 *     // Write header
 *     // Write union-find structure
 *     // Write all e-nodes
 *     // Write hash consing table
 * }
 *
 * eqx_egraph_t* deserialize_egraph(FILE *fp) {
 *     // Read header
 *     // Reconstruct union-find
 *     // Reconstruct e-nodes
 *     // Rebuild hash consing
 * }
 * @endcode
 */

/**
 * @page faq FAQ
 *
 * @section faq_what What is an E-Graph?
 *
 * An e-graph (equality graph) is a data structure that compactly represents
 * a set of terms and equivalence relations between them. It enables efficient
 * equality saturation, a technique for program optimization and theorem proving.
 *
 * @section faq_when When Should I Use E-Graphs?
 *
 * E-graphs are useful when you need to:
 * - Apply many rewrite rules exhaustively
 * - Explore multiple equivalent representations simultaneously
 * - Optimize programs or expressions
 * - Implement decision procedures for equality logic
 *
 * @section faq_perf How Fast is Equinox?
 *
 * Performance depends on workload, but typical operations:
 * - Term addition: O(1) amortized (hash consing)
 * - Union: O(α(n)) amortized (union-find with path compression)
 * - Congruence closure: O(n log n) per rebuild
 * - Pattern matching: O(nodes × patterns)
 *
 * @section faq_memory How Much Memory Does It Use?
 *
 * Memory usage is proportional to:
 * - Number of unique terms (e-nodes)
 * - Number of equivalence classes (e-classes)
 * - Hash table size (configurable)
 *
 * Hash consing ensures each unique term is stored only once.
 *
 * @section faq_limits What Are the Limits?
 *
 * - Maximum e-classes: Limited by `eqx_eclass_id_t` (typically 2^32)
 * - Maximum children per node: Limited by `size_t`
 * - Rebuild iterations: Configurable (default: 100)
 *
 * @section faq_thread Is It Thread-Safe?
 *
 * No. Each e-graph instance should be accessed by a single thread.
 * Use external synchronization if needed.
 *
 * @section faq_compare How Does It Compare to egg?
 *
 * Equinox is inspired by the egg library (Rust) but implemented in C:
 * - Similar core algorithms (union-find, congruence closure)
 * - C API for embedding in other languages
 * - Manual memory management
 * - No built-in e-class analysis (extensible by user)
 */

/**
 * @page changelog Changelog
 *
 * @section v0_1_0 Version 0.1.0 (Initial Release)
 *
 * - Core e-graph implementation
 * - Union-find with path compression
 * - Hash consing for structural sharing
 * - Congruence closure algorithm
 * - Pattern matching and rewriting
 * - Comprehensive test suite
 * - Documentation and examples
 */

#endif /* EQUINOX_DOXYGEN_I */
