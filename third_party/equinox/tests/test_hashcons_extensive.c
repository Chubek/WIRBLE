#include <catch2/catch_test_macros.hpp>
#include "equinox/hashcons.h"

TEST_CASE("hashcons creates and destroys", "[hashcons-extensive]") {
    eqx_hashcons_t *hc = eqx_hashcons_create(4, 0.75);
    REQUIRE(hc != NULL);
    REQUIRE(eqx_hashcons_size(hc) == 0);
    eqx_hashcons_destroy(hc);
}

TEST_CASE("hashcons inserts and looks up identical constants", "[hashcons-extensive]") {
    eqx_hashcons_t *hc = eqx_hashcons_create(4, 0.75);
    eqx_enode_t *n1 = eqx_enode_create(10, 0, NULL);
    eqx_enode_t *n2 = eqx_enode_create(10, 0, NULL);

    REQUIRE(eqx_hashcons_insert(hc, n1, 100));
    eqx_eclass_id_t lookup_id = EQX_ECLASS_ID_INVALID;
    REQUIRE(eqx_hashcons_lookup(hc, n2, &lookup_id));
    REQUIRE(lookup_id == 100);

    eqx_enode_t *n3 = eqx_enode_create(10, 0, NULL);
    REQUIRE(eqx_hashcons_lookup(hc, n3, NULL));
    REQUIRE_FALSE(eqx_hashcons_insert(hc, n1, 200));

    eqx_eclass_id_t id = EQX_ECLASS_ID_INVALID;
    REQUIRE(eqx_hashcons_lookup(hc, n1, &id));
    REQUIRE(id == 100);

    eqx_enode_destroy(n1);
    eqx_enode_destroy(n2);
    eqx_enode_destroy(n3);
    eqx_hashcons_destroy(hc);
}

TEST_CASE("hashcons inserts and removes nodes", "[hashcons-extensive]") {
    eqx_hashcons_t *hc = eqx_hashcons_create(4, 0.75);
    eqx_enode_t *node = eqx_enode_create(20, 0, NULL);

    REQUIRE(eqx_hashcons_insert(hc, node, 7));
    REQUIRE(eqx_hashcons_contains(hc, node));

    REQUIRE(eqx_hashcons_remove(hc, node));
    REQUIRE_FALSE(eqx_hashcons_contains(hc, node));
    REQUIRE(eqx_hashcons_size(hc) == 0);

    eqx_enode_destroy(node);
    eqx_hashcons_destroy(hc);
}

TEST_CASE("hashcons update changes existing mapping", "[hashcons-extensive]") {
    eqx_hashcons_t *hc = eqx_hashcons_create(4, 0.75);
    eqx_enode_t *node = eqx_enode_create(30, 0, NULL);

    REQUIRE(eqx_hashcons_insert(hc, node, 1));
    eqx_hashcons_update(hc, node, 33);

    eqx_eclass_id_t id = 0;
    REQUIRE(eqx_hashcons_lookup(hc, node, &id));
    REQUIRE(id == 33);

    eqx_enode_destroy(node);
    eqx_hashcons_destroy(hc);
}

TEST_CASE("hashcons handles explicit lookup misses", "[hashcons-extensive]") {
    eqx_hashcons_t *hc = eqx_hashcons_create(4, 0.75);
    eqx_enode_t *node1 = eqx_enode_create(40, 0, NULL);
    eqx_enode_t *node2 = eqx_enode_create(41, 0, NULL);

    REQUIRE_FALSE(eqx_hashcons_lookup(hc, node1, NULL));
    REQUIRE(eqx_hashcons_lookup(hc, node2, NULL) == false);

    eqx_enode_destroy(node1);
    eqx_enode_destroy(node2);
    eqx_hashcons_destroy(hc);
}

TEST_CASE("hashcons collision scenario via tiny buckets", "[hashcons-extensive]") {
    eqx_hashcons_t *hc = eqx_hashcons_create(1, 0.75);
    REQUIRE(hc != NULL);

    eqx_enode_t *nodes[12];
    for (int i = 0; i < 12; ++i) {
        eqx_eclass_id_t children[] = { (eqx_eclass_id_t)i };
        nodes[i] = eqx_enode_create((uint32_t)(100 + (i % 3)), 1, children);
        REQUIRE(eqx_hashcons_insert(hc, nodes[i], 1000 + i));
    }

    for (int i = 0; i < 12; ++i) {
        eqx_eclass_id_t id = EQX_ECLASS_ID_INVALID;
        REQUIRE(eqx_hashcons_lookup(hc, nodes[i], &id));
        REQUIRE(id == (eqx_eclass_id_t)(1000 + i));
    }

    for (int i = 0; i < 12; ++i) {
        eqx_enode_destroy(nodes[i]);
    }
    eqx_hashcons_destroy(hc);
}

TEST_CASE("hashcons triggers resize and keeps deterministic lookups", "[hashcons-extensive]") {
    eqx_hashcons_t *hc = eqx_hashcons_create(2, 0.50);
    eqx_enode_t *nodes[32];

    for (int i = 0; i < 32; ++i) {
        nodes[i] = eqx_enode_create((uint32_t)i, 0, NULL);
        REQUIRE(eqx_hashcons_insert(hc, nodes[i], (eqx_eclass_id_t)(i + 10)));
    }
    REQUIRE(eqx_hashcons_size(hc) == 32);

    for (int i = 0; i < 32; ++i) {
        eqx_eclass_id_t id = EQX_ECLASS_ID_INVALID;
        REQUIRE(eqx_hashcons_lookup(hc, nodes[i], &id));
        REQUIRE(id == (eqx_eclass_id_t)(i + 10));
    }

    for (int i = 0; i < 32; ++i) {
        eqx_enode_destroy(nodes[i]);
    }
    eqx_hashcons_destroy(hc);
}

TEST_CASE("hashcons iterator visits all entries", "[hashcons-extensive]") {
    eqx_hashcons_t *hc = eqx_hashcons_create(8, 0.75);
    for (uint32_t i = 0; i < 10; ++i) {
        eqx_enode_t *node = eqx_enode_create(i, 0, NULL);
        REQUIRE(eqx_hashcons_insert(hc, node, i + 1));
    }

    size_t count = 0;
    eqx_hashcons_iter_t it = eqx_hashcons_iter_begin(hc);
    while (eqx_hashcons_iter_has_next(it)) {
        eqx_enode_t *node = NULL;
        eqx_eclass_id_t id = EQX_ECLASS_ID_INVALID;
        eqx_hashcons_iter_next(it, &node, &id);
        REQUIRE(node != NULL);
        REQUIRE(id != EQX_ECLASS_ID_INVALID);
        ++count;
    }
    eqx_hashcons_iter_end(it);

    REQUIRE(count == 10);
    eqx_hashcons_destroy(hc);
}

TEST_CASE("hashcons deduplicates non-structural differences", "[hashcons-extensive]") {
    eqx_hashcons_t *hc = eqx_hashcons_create(8, 0.75);

    eqx_eclass_id_t c1[] = {1, 2};
    eqx_eclass_id_t c2[] = {1, 3};
    eqx_enode_t *a = eqx_enode_create(90, 2, c1);
    eqx_enode_t *b = eqx_enode_create(90, 2, c2);
    eqx_enode_t *c = eqx_enode_create(90, 2, c1);

    REQUIRE(eqx_hashcons_insert(hc, a, 500));
    REQUIRE_FALSE(eqx_hashcons_insert(hc, c, 501));
    REQUIRE(eqx_hashcons_insert(hc, b, 502));

    eqx_eclass_id_t id;
    REQUIRE(eqx_hashcons_lookup(hc, c, &id));
    REQUIRE(id == 500);
    REQUIRE(eqx_hashcons_lookup(hc, b, &id));
    REQUIRE(id == 502);

    eqx_enode_destroy(a);
    eqx_enode_destroy(b);
    eqx_enode_destroy(c);
    eqx_hashcons_destroy(hc);
}

TEST_CASE("hashcons remove missing node safely", "[hashcons-extensive]") {
    eqx_hashcons_t *hc = eqx_hashcons_create(8, 0.75);
    eqx_enode_t *node = eqx_enode_create(55, 0, NULL);
    eqx_enode_t *missing = eqx_enode_create(56, 0, NULL);

    REQUIRE(eqx_hashcons_insert(hc, node, 1));
    REQUIRE_FALSE(eqx_hashcons_remove(hc, missing));
    REQUIRE(eqx_hashcons_remove(hc, node));

    eqx_enode_destroy(node);
    eqx_enode_destroy(missing);
    eqx_hashcons_destroy(hc);
}

TEST_CASE("hashcons deterministic across tables and runs", "[hashcons-extensive]") {
    eqx_hashcons_t *a = eqx_hashcons_create(8, 0.75);
    eqx_hashcons_t *b = eqx_hashcons_create(8, 0.75);

    eqx_enode_t *n1 = eqx_enode_create(7, 0, NULL);
    eqx_enode_t *n2 = eqx_enode_create(8, 0, NULL);
    eqx_enode_t *n3 = eqx_enode_create(9, 0, NULL);

    REQUIRE(eqx_hashcons_insert(a, n1, 11));
    REQUIRE(eqx_hashcons_insert(a, n2, 22));
    REQUIRE(eqx_hashcons_insert(a, n3, 33));
    REQUIRE(eqx_hashcons_insert(b, n1, 11));
    REQUIRE(eqx_hashcons_insert(b, n2, 22));
    REQUIRE(eqx_hashcons_insert(b, n3, 33));

    eqx_eclass_id_t r1 = 0, r2 = 0, r3 = 0;
    REQUIRE(eqx_hashcons_lookup(a, n1, &r1));
    REQUIRE(eqx_hashcons_lookup(a, n2, &r2));
    REQUIRE(eqx_hashcons_lookup(a, n3, &r3));

    eqx_eclass_id_t s1 = 0, s2 = 0, s3 = 0;
    REQUIRE(eqx_hashcons_lookup(b, n1, &s1));
    REQUIRE(eqx_hashcons_lookup(b, n2, &s2));
    REQUIRE(eqx_hashcons_lookup(b, n3, &s3));

    REQUIRE(r1 == s1);
    REQUIRE(r2 == s2);
    REQUIRE(r3 == s3);

    eqx_enode_destroy(n1);
    eqx_enode_destroy(n2);
    eqx_enode_destroy(n3);
    eqx_hashcons_destroy(a);
    eqx_hashcons_destroy(b);
}

TEST_CASE("hashcons accepts application nodes with shared children", "[hashcons-extensive]") {
    eqx_hashcons_t *hc = eqx_hashcons_create(16, 0.75);

    eqx_eclass_id_t shared = 1;
    eqx_eclass_id_t children[] = {shared, shared, shared};
    eqx_enode_t *n = eqx_enode_create(66, 3, children);

    REQUIRE(eqx_hashcons_insert(hc, n, 900));
    REQUIRE_FALSE(eqx_hashcons_insert(hc, eqx_enode_create(66, 3, children), 901));

    eqx_hashcons_destroy(hc);
    eqx_enode_destroy(n);
}
