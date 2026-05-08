#include <catch2/catch_test_macros.hpp>
#include "equinox/rewrite.h"

static bool cond_true(const eqx_subst_t *subst, void *ctx) {
    (void)subst;
    (void)ctx;
    return true;
}

static bool cond_false(const eqx_subst_t *subst, void *ctx) {
    (void)subst;
    (void)ctx;
    return false;
}

TEST_CASE("rewrite variable matching binds binding", "[rewrite-extensive]") {
    eqx_egraph_t *g = eqx_egraph_create(NULL);
    eqx_pattern_t *pat = eqx_pattern_var("x");
    eqx_subst_t *s = eqx_subst_create();

    eqx_eclass_id_t leaf = eqx_egraph_add(g, 1, 0, NULL);
    REQUIRE(eqx_pattern_match(pat, g, leaf, s));

    eqx_eclass_id_t seen = EQX_ECLASS_ID_INVALID;
    REQUIRE(eqx_subst_lookup(s, "x", &seen));
    REQUIRE(seen == leaf);

    eqx_pattern_destroy(pat);
    eqx_subst_destroy(s);
    eqx_egraph_destroy(g);
}

TEST_CASE("rewrite app matching binds ordered children", "[rewrite-extensive]") {
    eqx_egraph_t *g = eqx_egraph_create(NULL);
    eqx_eclass_id_t l = eqx_egraph_add(g, 10, 0, NULL);
    eqx_eclass_id_t r = eqx_egraph_add(g, 11, 0, NULL);
    eqx_eclass_id_t term = eqx_egraph_add(g, 99, 2, (eqx_eclass_id_t[]){l, r});

    eqx_pattern_t *lhs = eqx_pattern_app(99,
        (eqx_pattern_t*[]){eqx_pattern_var("left"), eqx_pattern_var("right")},
        2);
    eqx_subst_t *s = eqx_subst_create();

    REQUIRE(eqx_pattern_match(lhs, g, term, s));
    eqx_eclass_id_t got_left = 0;
    eqx_eclass_id_t got_right = 0;
    REQUIRE(eqx_subst_lookup(s, "left", &got_left));
    REQUIRE(eqx_subst_lookup(s, "right", &got_right));
    REQUIRE(got_left == l);
    REQUIRE(got_right == r);

    eqx_pattern_destroy(lhs);
    eqx_subst_destroy(s);
    eqx_egraph_destroy(g);
}

TEST_CASE("rewrite app matching rejects operator mismatch", "[rewrite-extensive]") {
    eqx_egraph_t *g = eqx_egraph_create(NULL);
    eqx_eclass_id_t a = eqx_egraph_add(g, 1, 0, NULL);
    eqx_eclass_id_t b = eqx_egraph_add(g, 2, 0, NULL);
    eqx_eclass_id_t term = eqx_egraph_add(g, 99, 2, (eqx_eclass_id_t[]){a, b});

    eqx_pattern_t *lhs = eqx_pattern_app(100,
        (eqx_pattern_t*[]){eqx_pattern_var("x"), eqx_pattern_var("y")},
        2);
    eqx_subst_t *s = eqx_subst_create();
    REQUIRE_FALSE(eqx_pattern_match(lhs, g, term, s));

    eqx_pattern_destroy(lhs);
    eqx_subst_destroy(s);
    eqx_egraph_destroy(g);
}

TEST_CASE("rewrite substitution lookup failure", "[rewrite-extensive]") {
    eqx_subst_t *s = eqx_subst_create();
    eqx_eclass_id_t id = EQX_ECLASS_ID_INVALID;
    REQUIRE_FALSE(eqx_subst_lookup(s, "missing", &id));
    eqx_subst_destroy(s);
}

TEST_CASE("rewrite instantiates variable pattern", "[rewrite-extensive]") {
    eqx_egraph_t *g = eqx_egraph_create(NULL);
    eqx_subst_t *s = eqx_subst_create();
    eqx_eclass_id_t leaf = eqx_egraph_add(g, 1, 0, NULL);
    REQUIRE(eqx_subst_add(s, "x", leaf));

    eqx_pattern_t *pat = eqx_pattern_var("x");
    REQUIRE(eqx_pattern_instantiate(pat, g, s) == leaf);

    eqx_pattern_destroy(pat);
    eqx_subst_destroy(s);
    eqx_egraph_destroy(g);
}

TEST_CASE("rewrite fails instantiate without binding", "[rewrite-extensive]") {
    eqx_egraph_t *g = eqx_egraph_create(NULL);
    eqx_subst_t *s = eqx_subst_create();
    eqx_pattern_t *pat = eqx_pattern_var("x");

    REQUIRE(eqx_pattern_instantiate(pat, g, s) == EQX_ECLASS_ID_INVALID);

    eqx_pattern_destroy(pat);
    eqx_subst_destroy(s);
    eqx_egraph_destroy(g);
}

TEST_CASE("rewrite applies matching rule", "[rewrite-extensive]") {
    eqx_egraph_t *g = eqx_egraph_create(NULL);
    eqx_eclass_id_t leaf = eqx_egraph_add(g, 7, 0, NULL);
    eqx_pattern_t *lhs = eqx_pattern_app(100,
        (eqx_pattern_t*[]){eqx_pattern_var("x")}, 1);
    eqx_pattern_t *rhs = eqx_pattern_app(200,
        (eqx_pattern_t*[]){eqx_pattern_var("x")}, 1);
    eqx_rewrite_rule_t *r = eqx_rewrite_rule_create("r1", lhs, rhs, NULL);

    eqx_eclass_id_t term = eqx_egraph_add(g, 100, 1, (eqx_eclass_id_t[]){leaf});
    REQUIRE(eqx_rewrite_apply(r, g, term) == 1);

    eqx_rewrite_rule_destroy(r);
    eqx_egraph_destroy(g);
}

TEST_CASE("rewrite no-match rule returns zero", "[rewrite-extensive]") {
    eqx_egraph_t *g = eqx_egraph_create(NULL);
    eqx_eclass_id_t leaf = eqx_egraph_add(g, 9, 0, NULL);

    eqx_rewrite_rule_t *r = eqx_rewrite_rule_create(
        "r",
        eqx_pattern_app(100, (eqx_pattern_t*[]){eqx_pattern_var("x")}, 1),
        eqx_pattern_var("x"),
        NULL);

    REQUIRE(eqx_rewrite_apply(r, g, leaf) == 0);

    eqx_rewrite_rule_destroy(r);
    eqx_egraph_destroy(g);
}

TEST_CASE("rewrite variable reuse enforces consistency", "[rewrite-extensive]") {
    eqx_egraph_t *g = eqx_egraph_create(NULL);
    eqx_eclass_id_t a = eqx_egraph_add(g, 1, 0, NULL);
    eqx_eclass_id_t term = eqx_egraph_add(g, 100, 2, (eqx_eclass_id_t[]){a, a});

    eqx_pattern_t *vars[] = {eqx_pattern_var("x"), eqx_pattern_var("x")};
    eqx_pattern_t *lhs = eqx_pattern_app(100, vars, 2);
    eqx_rewrite_rule_t *r = eqx_rewrite_rule_create(
        "same",
        lhs,
        eqx_pattern_var("x"),
        NULL);

    REQUIRE(eqx_rewrite_apply(r, g, term) == 1);

    eqx_rewrite_rule_destroy(r);
    eqx_egraph_destroy(g);
}

TEST_CASE("rewrite nested pattern matching", "[rewrite-extensive]") {
    eqx_egraph_t *g = eqx_egraph_create(NULL);
    eqx_eclass_id_t a = eqx_egraph_add(g, 1, 0, NULL);
    eqx_eclass_id_t b = eqx_egraph_add(g, 2, 0, NULL);
    eqx_eclass_id_t pair = eqx_egraph_add(g, 20, 2, (eqx_eclass_id_t[]){a, b});
    eqx_eclass_id_t term = eqx_egraph_add(g, 30, 1, (eqx_eclass_id_t[]){pair});

    eqx_pattern_t *inner = eqx_pattern_app(20,
        (eqx_pattern_t*[]){eqx_pattern_var("x"), eqx_pattern_var("y")},
        2);
    eqx_pattern_t *outer = eqx_pattern_app(30, (eqx_pattern_t*[]){inner}, 1);
    eqx_subst_t *s = eqx_subst_create();

    REQUIRE(eqx_pattern_match(outer, g, term, s));

    eqx_pattern_destroy(outer);
    eqx_subst_destroy(s);
    eqx_egraph_destroy(g);
}

TEST_CASE("rewrite apply_all scans all roots", "[rewrite-extensive]") {
    eqx_egraph_t *g = eqx_egraph_create(NULL);
    eqx_rewrite_rule_t *r = eqx_rewrite_rule_create(
        "all",
        eqx_pattern_var("x"),
        eqx_pattern_var("x"),
        cond_true);

    for (uint32_t i = 0; i < 20; ++i) {
        eqx_egraph_add(g, 1000 + i, 0, NULL);
    }

    REQUIRE(eqx_rewrite_apply_all(r, g) > 0);

    eqx_rewrite_rule_destroy(r);
    eqx_egraph_destroy(g);
}

TEST_CASE("rewrite apply_rules drives fixed iterations", "[rewrite-extensive]") {
    eqx_egraph_t *g = eqx_egraph_create(NULL);

    eqx_rewrite_rule_t *r1 = eqx_rewrite_rule_create(
        "r1",
        eqx_pattern_app(1, (eqx_pattern_t*[]){eqx_pattern_var("x")}, 1),
        eqx_pattern_app(2, (eqx_pattern_t*[]){eqx_pattern_var("x")}, 1),
        NULL);
    eqx_rewrite_rule_t *r2 = eqx_rewrite_rule_create(
        "r2",
        eqx_pattern_app(2, (eqx_pattern_t*[]){eqx_pattern_var("x")}, 1),
        eqx_pattern_app(3, (eqx_pattern_t*[]){eqx_pattern_var("x")}, 1),
        NULL);

    eqx_rewrite_rule_t *rules[] = {r1, r2};
    eqx_eclass_id_t leaf = eqx_egraph_add(g, 8, 0, NULL);
    eqx_egraph_add(g, 1, 1, (eqx_eclass_id_t[]){leaf});

    REQUIRE(eqx_rewrite_apply_rules(rules, 2, g, 4) > 0);
    eqx_egraph_rebuild(g);
    REQUIRE(eqx_egraph_find(g, eqx_egraph_add(g, 3, 1, (eqx_eclass_id_t[]){leaf}) ) != EQX_ECLASS_ID_INVALID);

    eqx_rewrite_rule_destroy(r1);
    eqx_rewrite_rule_destroy(r2);
    eqx_egraph_destroy(g);
}

TEST_CASE("rewrite overlapping patterns interact", "[rewrite-extensive]") {
    eqx_egraph_t *g = eqx_egraph_create(NULL);

    eqx_rewrite_rule_t *r1 = eqx_rewrite_rule_create(
        "f-to-g",
        eqx_pattern_app(5, (eqx_pattern_t*[]){eqx_pattern_var("x")}, 1),
        eqx_pattern_app(6, (eqx_pattern_t*[]){eqx_pattern_var("x")}, 1),
        NULL);
    eqx_rewrite_rule_t *r2 = eqx_rewrite_rule_create(
        "f-to-h",
        eqx_pattern_app(5, (eqx_pattern_t*[]){eqx_pattern_var("x")}, 1),
        eqx_pattern_app(7, (eqx_pattern_t*[]){eqx_pattern_var("x")}, 1),
        NULL);

    eqx_eclass_id_t leaf = eqx_egraph_add(g, 1, 0, NULL);
    eqx_eclass_id_t term = eqx_egraph_add(g, 5, 1, (eqx_eclass_id_t[]){leaf});
    size_t out = eqx_rewrite_apply(r1, g, term);
    out += eqx_rewrite_apply(r2, g, term);
    REQUIRE(out >= 1);

    eqx_rewrite_rule_destroy(r1);
    eqx_rewrite_rule_destroy(r2);
    eqx_egraph_destroy(g);
}

TEST_CASE("rewrite rule condition can block rewrite", "[rewrite-extensive]") {
    eqx_egraph_t *g = eqx_egraph_create(NULL);
    eqx_rewrite_rule_t *r = eqx_rewrite_rule_create(
        "cond",
        eqx_pattern_app(10, (eqx_pattern_t*[]){eqx_pattern_var("x")}, 1),
        eqx_pattern_app(11, (eqx_pattern_t*[]){eqx_pattern_var("x")}, 1),
        cond_false);

    eqx_eclass_id_t leaf = eqx_egraph_add(g, 1, 0, NULL);
    eqx_eclass_id_t term = eqx_egraph_add(g, 10, 1, (eqx_eclass_id_t[]){leaf});
    REQUIRE(eqx_rewrite_apply(r, g, term) == 0);

    eqx_rewrite_rule_destroy(r);
    eqx_egraph_destroy(g);
}

TEST_CASE("rewrite rule condition can permit rewrite", "[rewrite-extensive]") {
    eqx_egraph_t *g = eqx_egraph_create(NULL);
    eqx_rewrite_rule_t *r = eqx_rewrite_rule_create(
        "cond",
        eqx_pattern_app(10, (eqx_pattern_t*[]){eqx_pattern_var("x")}, 1),
        eqx_pattern_app(11, (eqx_pattern_t*[]){eqx_pattern_var("x")}, 1),
        cond_true);

    eqx_eclass_id_t leaf = eqx_egraph_add(g, 1, 0, NULL);
    eqx_eclass_id_t term = eqx_egraph_add(g, 10, 1, (eqx_eclass_id_t[]){leaf});
    REQUIRE(eqx_rewrite_apply(r, g, term) == 1);

    eqx_rewrite_rule_destroy(r);
    eqx_egraph_destroy(g);
}

TEST_CASE("rewrite apply_rules terminates under max iterations", "[rewrite-extensive]") {
    eqx_egraph_t *g = eqx_egraph_create(NULL);
    eqx_rewrite_rule_t *r = eqx_rewrite_rule_create(
        "idem",
        eqx_pattern_var("x"),
        eqx_pattern_var("x"),
        NULL);

    eqx_egraph_add(g, 1, 0, NULL);
    eqx_rewrite_rule_t *rules[] = {r};
    REQUIRE(eqx_rewrite_apply_rules(rules, 1, g, 1) > 0);

    eqx_rewrite_rule_destroy(r);
    eqx_egraph_destroy(g);
}
