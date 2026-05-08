/* tests/test_rewrite.c */
#include <catch2/catch_test_macros.hpp>
#include "equinox/rewrite.h"
#include "equinox/egraph.h"

TEST_CASE("pattern creation", "[rewrite]") {
    SECTION("create variable pattern") {
        eqx_pattern_t *pat = eqx_pattern_var("x");
        REQUIRE(pat != NULL);
        REQUIRE(eqx_pattern_is_var(pat));
        eqx_pattern_destroy(pat);
    }
    
    SECTION("create application pattern") {
        eqx_pattern_t *x = eqx_pattern_var("x");
        eqx_pattern_t *y = eqx_pattern_var("y");
        
        eqx_pattern_t *children[] = {x, y};
        eqx_pattern_t *app = eqx_pattern_app(100, children, 2);
        
        REQUIRE(app != NULL);
        REQUIRE_FALSE(eqx_pattern_is_var(app));
        
        eqx_pattern_destroy(app);
    }
}

TEST_CASE("substitution operations", "[rewrite]") {
    SECTION("create empty substitution") {
        eqx_subst_t *subst = eqx_subst_create();
        REQUIRE(subst != NULL);
        eqx_subst_destroy(subst);
    }
    
    SECTION("add and lookup binding") {
        eqx_subst_t *subst = eqx_subst_create();
        
        eqx_subst_add(subst, "x", 100);
        
        eqx_eclass_id_t id;
        bool found = eqx_subst_lookup(subst, "x", &id);
        REQUIRE(found);
        REQUIRE(id == 100);
        
        eqx_subst_destroy(subst);
    }
    
    SECTION("lookup non-existing variable") {
        eqx_subst_t *subst = eqx_subst_create();
        
        eqx_eclass_id_t id;
        bool found = eqx_subst_lookup(subst, "y", &id);
        REQUIRE_FALSE(found);
        
        eqx_subst_destroy(subst);
    }
    
    SECTION("multiple bindings") {
        eqx_subst_t *subst = eqx_subst_create();
        
        eqx_subst_add(subst, "x", 100);
        eqx_subst_add(subst, "y", 200);
        eqx_subst_add(subst, "z", 300);
        
        eqx_eclass_id_t id_x, id_y, id_z;
        REQUIRE(eqx_subst_lookup(subst, "x", &id_x));
        REQUIRE(eqx_subst_lookup(subst, "y", &id_y));
        REQUIRE(eqx_subst_lookup(subst, "z", &id_z));
        
        REQUIRE(id_x == 100);
        REQUIRE(id_y == 200);
        REQUIRE(id_z == 300);
        
        eqx_subst_destroy(subst);
    }
}

TEST_CASE("pattern matching", "[rewrite]") {
    eqx_egraph_config_t config = eqx_egraph_config_default();
    eqx_egraph_t *egraph = eqx_egraph_create(&config);
    
    SECTION("match variable pattern") {
        eqx_eclass_id_t id = eqx_egraph_add(egraph, 42, NULL, 0);
        
        eqx_pattern_t *pat = eqx_pattern_var("x");
        eqx_subst_t *subst = eqx_subst_create();
        
        bool matched = eqx_pattern_match(pat, egraph, id, subst);
        REQUIRE(matched);
        
        eqx_eclass_id_t bound_id;
        REQUIRE(eqx_subst_lookup(subst, "x", &bound_id));
        REQUIRE(bound_id == id);
        
        eqx_pattern_destroy(pat);
        eqx_subst_destroy(subst);
    }
    
    SECTION("match application pattern") {
        eqx_eclass_id_t c1 = eqx_egraph_add(egraph, 1, NULL, 0);
        eqx_eclass_id_t c2 = eqx_egraph_add(egraph, 2, NULL, 0);
        
        eqx_eclass_id_t children[] = {c1, c2};
        eqx_eclass_id_t app = eqx_egraph_add(egraph, 100, children, 2);
        
        eqx_pattern_t *x = eqx_pattern_var("x");
        eqx_pattern_t *y = eqx_pattern_var("y");
        eqx_pattern_t *pat_children[] = {x, y};
        eqx_pattern_t *pat = eqx_pattern_app(100, pat_children, 2);
        
        eqx_subst_t *subst = eqx_subst_create();
        bool matched = eqx_pattern_match(pat, egraph, app, subst);
        REQUIRE(matched);
        
        eqx_eclass_id_t x_id, y_id;
        REQUIRE(eqx_subst_lookup(subst, "x", &x_id));
        REQUIRE(eqx_subst_lookup(subst, "y", &y_id));
        REQUIRE(x_id == c1);
        REQUIRE(y_id == c2);
        
        eqx_pattern_destroy(pat);
        eqx_subst_destroy(subst);
    }
    
    SECTION("match fails on wrong operator") {
        eqx_eclass_id_t c1 = eqx_egraph_add(egraph, 1, NULL, 0);
        eqx_eclass_id_t c2 = eqx_egraph_add(egraph, 2, NULL, 0);
        
        eqx_eclass_id_t children[] = {c1, c2};
        eqx_eclass_id_t app = eqx_egraph_add(egraph, 100, children, 2);
        
        eqx_pattern_t *x = eqx_pattern_var("x");
        eqx_pattern_t *y = eqx_pattern_var("y");
        eqx_pattern_t *pat_children[] = {x, y};
        eqx_pattern_t *pat = eqx_pattern_app(200, pat_children, 2);
        
        eqx_subst_t *subst = eqx_subst_create();
        bool matched = eqx_pattern_match(pat, egraph, app, subst);
        REQUIRE_FALSE(matched);
        
        eqx_pattern_destroy(pat);
        eqx_subst_destroy(subst);
    }
    
    eqx_egraph_destroy(egraph);
}

TEST_CASE("pattern instantiation", "[rewrite]") {
    eqx_egraph_config_t config = eqx_egraph_config_default();
    eqx_egraph_t *egraph = eqx_egraph_create(&config);
    
    SECTION("instantiate variable pattern") {
        eqx_subst_t *subst = eqx_subst_create();
        eqx_subst_add(subst, "x", 100);
        
        eqx_pattern_t *pat = eqx_pattern_var("x");
        
        eqx_eclass_id_t id = eqx_pattern_instantiate(pat, egraph, subst);
        REQUIRE(id == 100);
        
        eqx_pattern_destroy(pat);
        eqx_subst_destroy(subst);
    }
    
    SECTION("instantiate application pattern") {
        eqx_eclass_id_t c1 = eqx_egraph_add(egraph, 1, NULL, 0);
        eqx_eclass_id_t c2 = eqx_egraph_add(egraph, 2, NULL, 0);
        
        eqx_subst_t *subst = eqx_subst_create();
        eqx_subst_add(subst, "x", c1);
        eqx_subst_add(subst, "y", c2);
        
        eqx_pattern_t *x = eqx_pattern_var("x");
        eqx_pattern_t *y = eqx_pattern_var("y");
        eqx_pattern_t *pat_children[] = {x, y};
        eqx_pattern_t *pat = eqx_pattern_app(100, pat_children, 2);
        
        eqx_eclass_id_t id = eqx_pattern_instantiate(pat, egraph, subst);
        REQUIRE(id != EQX_ECLASS_ID_INVALID);
        
        eqx_pattern_destroy(pat);
        eqx_subst_destroy(subst);
    }
    
    eqx_egraph_destroy(egraph);
}

TEST_CASE("rewrite rule creation", "[rewrite]") {
    SECTION("create simple rule") {
        eqx_pattern_t *lhs = eqx_pattern_var("x");
        eqx_pattern_t *rhs = eqx_pattern_var("x");
        
        eqx_rewrite_rule_t *rule = eqx_rewrite_rule_create("identity", lhs, rhs, NULL);
        REQUIRE(rule != NULL);
        
        eqx_rewrite_rule_destroy(rule);
    }
    
    SECTION("create rule with condition") {
        eqx_pattern_t *lhs = eqx_pattern_var("x");
        eqx_pattern_t *rhs = eqx_pattern_var("y");
        
        bool (*cond)(const eqx_subst_t*, void*) = [](const eqx_subst_t *s, void *ctx) {
            return true;
        };
        
        eqx_rewrite_rule_t *rule = eqx_rewrite_rule_create("conditional", lhs, rhs, cond);
        REQUIRE(rule != NULL);
        
        eqx_rewrite_rule_destroy(rule);
    }
}

TEST_CASE("rewrite rule application", "[rewrite]") {
    eqx_egraph_config_t config = eqx_egraph_config_default();
    eqx_egraph_t *egraph = eqx_egraph_create(&config);
    
    SECTION("apply simple rewrite") {
        // Create term: f(a)
        eqx_eclass_id_t a = eqx_egraph_add(egraph, 1, NULL, 0);
        eqx_eclass_id_t children[] = {a};
        eqx_eclass_id_t f_a = eqx_egraph_add(egraph, 100, children, 2);
        
        // Create rule: f(x) -> g(x)
        eqx_pattern_t *x_lhs = eqx_pattern_var("x");
        eqx_pattern_t *lhs_children[] = {x_lhs};
        eqx_pattern_t *lhs = eqx_pattern_app(100, lhs_children, 1);
        
        eqx_pattern_t *x_rhs = eqx_pattern_var("x");
        eqx_pattern_t *rhs_children[] = {x_rhs};
        eqx_pattern_t *rhs = eqx_pattern_app(200, rhs_children, 1);
        
        eqx_rewrite_rule_t *rule = eqx_rewrite_rule_create("f_to_g", lhs, rhs, NULL);
        
        // Apply rule
        bool applied = eqx_rewrite_apply(rule, egraph, f_a);
        REQUIRE(applied);
        
        eqx_rewrite_rule_destroy(rule);
    }
    
    eqx_egraph_destroy(egraph);
}

TEST_CASE("bulk rewriting", "[rewrite]") {
    eqx_egraph_config_t config = eqx_egraph_config_default();
    eqx_egraph_t *egraph = eqx_egraph_create(&config);
    
    SECTION("apply rules to all e-classes") {
        // Create multiple terms
        eqx_eclass_id_t a = eqx_egraph_add(egraph, 1, NULL, 0);
        eqx_eclass_id_t b = eqx_egraph_add(egraph, 2, NULL, 0);
        
        eqx_eclass_id_t children1[] = {a};
        eqx_eclass_id_t f_a = eqx_egraph_add(egraph, 100, children1, 1);
        
        eqx_eclass_id_t children2[] = {b};
        eqx_eclass_id_t f_b = eqx_egraph_add(egraph, 100, children2, 1);
        
        // Create rule: f(x) -> g(x)
        eqx_pattern_t *x_lhs = eqx_pattern_var("x");
        eqx_pattern_t *lhs_children[] = {x_lhs};
        eqx_pattern_t *lhs = eqx_pattern_app(100, lhs_children, 1);
        
        eqx_pattern_t *x_rhs = eqx_pattern_var("x");
        eqx_pattern_t *rhs_children[] = {x_rhs};
        eqx_pattern_t *rhs = eqx_pattern_app(200, rhs_children, 1);
        
        eqx_rewrite_rule_t *rule = eqx_rewrite_rule_create("f_to_g", lhs, rhs, NULL);
        
        eqx_rewrite_rule_t *rules[] = {rule};
        
        // Apply rules to all e-classes
        size_t num_applied = eqx_rewrite_apply_all(egraph, rules, 1, 10);
        REQUIRE(num_applied > 0);
        
        eqx_rewrite_rule_destroy(rule);
    }
    
    eqx_egraph_destroy(egraph);
}
