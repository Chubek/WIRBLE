#include <catch2/catch_test_macros.hpp>
#include <stdint.h>
#include "equinox/egraph.h"

static eqx_egraph_t* create_graph(void) {
    return eqx_egraph_create(NULL);
}

TEST_CASE("egraph constant hashcons deduplicates by operator and arity", "[egraph-extensive]") {
    eqx_egraph_t *g = create_graph();
    REQUIRE(g != NULL);

    eqx_eclass_id_t a1 = eqx_egraph_add(g, 42, 0, NULL);
    eqx_eclass_id_t a2 = eqx_egraph_add(g, 42, 0, NULL);
    eqx_eclass_id_t b = eqx_egraph_add(g, 43, 0, NULL);

    REQUIRE(a1 != EQX_ECLASS_ID_INVALID);
    REQUIRE(a1 == a2);
    REQUIRE(a1 != b);

    eqx_egraph_destroy(g);
}

TEST_CASE("egraph deduplicates identical applications", "[egraph-extensive]") {
    eqx_egraph_t *g = create_graph();

    eqx_eclass_id_t a = eqx_egraph_add(g, 1, 0, NULL);
    eqx_eclass_id_t b = eqx_egraph_add(g, 2, 0, NULL);
    eqx_eclass_id_t children[] = {a, b};
    eqx_eclass_id_t f1 = eqx_egraph_add(g, 100, 2, children);
    eqx_eclass_id_t f2 = eqx_egraph_add(g, 100, 2, children);

    REQUIRE(f1 == f2);
    eqx_egraph_destroy(g);
}

TEST_CASE("egraph canonicalizes child IDs at insertion", "[egraph-extensive]") {
    eqx_egraph_t *g = create_graph();

    eqx_eclass_id_t leaf = eqx_egraph_add(g, 10, 0, NULL);
    eqx_eclass_id_t leaf_copy = eqx_egraph_add(g, 10, 0, NULL);
    REQUIRE(leaf == leaf_copy);

    eqx_eclass_id_t a_children[] = {leaf};
    eqx_eclass_id_t app1 = eqx_egraph_add(g, 77, 1, a_children);
    eqx_egraph_union(g, leaf, leaf_copy);
    eqx_egraph_rebuild(g);
    eqx_eclass_id_t app2 = eqx_egraph_add(g, 77, 1, a_children);

    REQUIRE(app1 == app2);
    eqx_egraph_destroy(g);
}

TEST_CASE("egraph builds deep nested structures", "[egraph-extensive]") {
    eqx_egraph_t *g = create_graph();
    eqx_eclass_id_t id = eqx_egraph_add(g, 1, 0, NULL);

    for (uint32_t depth = 0; depth < 128; ++depth) {
        eqx_eclass_id_t children[] = {id};
        id = eqx_egraph_add(g, 5, 1, children);
    }

    REQUIRE(id != EQX_ECLASS_ID_INVALID);
    REQUIRE(eqx_egraph_find(g, id) == id);
    eqx_egraph_destroy(g);
}

TEST_CASE("egraph handles large DAGs above 1000 nodes", "[egraph-extensive]") {
    eqx_egraph_t *g = create_graph();
    const size_t nodes = 1024;
    eqx_eclass_id_t ids[nodes];

    ids[0] = eqx_egraph_add(g, 1, 0, NULL);
    for (size_t i = 1; i < nodes; ++i) {
        size_t a = i - 1;
        size_t b = (i >= 2) ? i - 2 : 0;
        eqx_eclass_id_t children[] = {ids[a], ids[b]};
        ids[i] = eqx_egraph_add(g, (uint32_t)(i % 16), 2, children);
        REQUIRE(ids[i] != EQX_ECLASS_ID_INVALID);
    }

    REQUIRE(ids[nodes - 1] != EQX_ECLASS_ID_INVALID);
    REQUIRE(eqx_egraph_find(g, ids[nodes - 1]) == ids[nodes - 1]);
    eqx_egraph_destroy(g);
}

TEST_CASE("egraph rebuild is idempotent when clean", "[egraph-extensive]") {
    eqx_egraph_t *g = create_graph();
    REQUIRE(eqx_egraph_rebuild(g));
    REQUIRE(eqx_egraph_rebuild(g));

    eqx_eclass_id_t a = eqx_egraph_add(g, 2, 0, NULL);
    eqx_eclass_id_t b = eqx_egraph_add(g, 3, 0, NULL);
    eqx_egraph_add(g, 99, 1, (eqx_eclass_id_t[]){a});
    eqx_egraph_union(g, a, b);
    eqx_egraph_rebuild(g);
    REQUIRE(eqx_egraph_rebuild(g));

    eqx_egraph_destroy(g);
}

TEST_CASE("egraph propagates congruence after union and rebuild", "[egraph-extensive]") {
    eqx_egraph_t *g = create_graph();

    eqx_eclass_id_t a = eqx_egraph_add(g, 1, 0, NULL);
    eqx_eclass_id_t b = eqx_egraph_add(g, 2, 0, NULL);
    eqx_eclass_id_t c = eqx_egraph_add(g, 3, 0, NULL);
    eqx_eclass_id_t d = eqx_egraph_add(g, 4, 0, NULL);
    eqx_eclass_id_t f1 = eqx_egraph_add(g, 77, 2, (eqx_eclass_id_t[]){a, b});
    eqx_eclass_id_t f2 = eqx_egraph_add(g, 77, 2, (eqx_eclass_id_t[]){c, d});

    REQUIRE_FALSE(eqx_egraph_equiv(g, f1, f2));
    eqx_egraph_union(g, a, c);
    eqx_egraph_union(g, b, d);
    eqx_egraph_rebuild(g);
    REQUIRE(eqx_egraph_equiv(g, f1, f2));

    eqx_egraph_destroy(g);
}

TEST_CASE("egraph preserves equivalence before and after rebuild", "[egraph-extensive]") {
    eqx_egraph_t *g = create_graph();

    eqx_eclass_id_t x = eqx_egraph_add(g, 11, 0, NULL);
    eqx_eclass_id_t y = eqx_egraph_add(g, 12, 0, NULL);
    eqx_eclass_id_t z = eqx_egraph_add(g, 13, 0, NULL);
    eqx_eclass_id_t f1 = eqx_egraph_add(g, 55, 2, (eqx_eclass_id_t[]){x, y});
    eqx_eclass_id_t f2 = eqx_egraph_add(g, 55, 2, (eqx_eclass_id_t[]){x, z});

    REQUIRE_FALSE(eqx_egraph_equiv(g, f1, f2));
    REQUIRE(eqx_egraph_union(g, y, z));
    REQUIRE(eqx_egraph_equiv(g, x, x));
    REQUIRE_FALSE(eqx_egraph_equiv(g, x, y));

    eqx_egraph_rebuild(g);
    REQUIRE(eqx_egraph_equiv(g, y, z));
    REQUIRE(eqx_egraph_equiv(g, f1, f2));

    eqx_egraph_destroy(g);
}

TEST_CASE("egraph tracks structural sharing via parent links", "[egraph-extensive]") {
    eqx_egraph_t *g = create_graph();
    eqx_eclass_id_t shared = eqx_egraph_add(g, 99, 0, NULL);

    eqx_eclass_id_t first = eqx_egraph_add(g, 7, 1, (eqx_eclass_id_t[]){shared});
    eqx_eclass_id_t second = eqx_egraph_add(g, 7, 1, (eqx_eclass_id_t[]){shared});
    REQUIRE(first == second);

    eqx_egraph_destroy(g);
}

TEST_CASE("egraph find invalid id returns invalid", "[egraph-extensive]") {
    eqx_egraph_t *g = create_graph();
    eqx_eclass_id_t a = eqx_egraph_add(g, 1, 0, NULL);

    REQUIRE(eqx_egraph_find(g, 999) == EQX_ECLASS_ID_INVALID);
    REQUIRE_FALSE(eqx_egraph_union(g, a, 999));
    REQUIRE(eqx_egraph_get_eclass(g, 999) == NULL);

    eqx_egraph_destroy(g);
}

TEST_CASE("egraph iterator returns canonical classes only", "[egraph-extensive]") {
    eqx_egraph_t *g = create_graph();
    eqx_eclass_id_t a = eqx_egraph_add(g, 1, 0, NULL);
    eqx_eclass_id_t b = eqx_egraph_add(g, 2, 0, NULL);
    REQUIRE(eqx_egraph_union(g, a, b));
    eqx_egraph_rebuild(g);

    size_t count = 0;
    eqx_egraph_iter_t it = eqx_egraph_iter_begin(g);
    while (eqx_egraph_iter_has_next(it)) {
        eqx_eclass_id_t id = eqx_egraph_iter_next(it);
        REQUIRE(id != EQX_ECLASS_ID_INVALID);
        REQUIRE(eqx_egraph_find(g, id) == id);
        ++count;
    }
    eqx_egraph_iter_end(it);

    REQUIRE(count >= 1);
    REQUIRE(count <= eqx_egraph_num_eclasses(g));
    eqx_egraph_destroy(g);
}

TEST_CASE("egraph stats reflect enodes and unions", "[egraph-extensive]") {
    eqx_egraph_t *g = create_graph();
    eqx_egraph_stats_t stats = {0};

    eqx_eclass_id_t a = eqx_egraph_add(g, 1, 0, NULL);
    eqx_eclass_id_t b = eqx_egraph_add(g, 2, 0, NULL);
    eqx_egraph_add(g, 3, 2, (eqx_eclass_id_t[]){a, b});
    eqx_egraph_get_stats(g, &stats);
    REQUIRE(stats.num_enodes >= 3);
    REQUIRE(stats.num_eclasses >= 3);

    eqx_egraph_union(g, a, b);
    eqx_egraph_get_stats(g, &stats);
    REQUIRE(stats.num_unions >= 1);

    eqx_egraph_destroy(g);
}

TEST_CASE("egraph invalid unions remain false", "[egraph-extensive]") {
    eqx_egraph_t *g = create_graph();

    eqx_eclass_id_t a = eqx_egraph_add(g, 1, 0, NULL);
    REQUIRE_FALSE(eqx_egraph_union(g, a, EQX_ECLASS_ID_INVALID));
    REQUIRE_FALSE(eqx_egraph_union(g, a, 9999));

    eqx_egraph_destroy(g);
}

TEST_CASE("egraph deterministic class IDs are monotonic", "[egraph-extensive]") {
    eqx_egraph_t *g = create_graph();

    eqx_eclass_id_t ids[6];
    for (uint32_t i = 0; i < 6; ++i) {
        ids[i] = eqx_egraph_add(g, 200 + i, 0, NULL);
        REQUIRE(ids[i] == i);
    }

    eqx_eclass_id_t dup = eqx_egraph_add(g, 201, 0, NULL);
    REQUIRE(dup == 1);
    eqx_egraph_destroy(g);
}
