#include <catch2/catch_test_macros.hpp>
#include "equinox/unionfind.h"

static eqx_unionfind_t* create_sets(size_t initial) {
    return eqx_unionfind_create(initial);
}

TEST_CASE("unionfind starts with expected capacity and grows", "[unionfind-extensive]") {
    eqx_unionfind_t *uf = create_sets(2);
    REQUIRE(uf != NULL);

    REQUIRE(eqx_unionfind_size(uf) == 0);
    REQUIRE(eqx_unionfind_num_sets(uf) == 0);

    eqx_eclass_id_t first = eqx_unionfind_make_set(uf);
    eqx_eclass_id_t second = eqx_unionfind_make_set(uf);
    eqx_eclass_id_t third = eqx_unionfind_make_set(uf);

    REQUIRE(first == 0);
    REQUIRE(second == 1);
    REQUIRE(third == 2);
    REQUIRE(eqx_unionfind_size(uf) == 3);

    eqx_unionfind_destroy(uf);
}

TEST_CASE("unionfind makes singleton find its own representative", "[unionfind-extensive]") {
    eqx_unionfind_t *uf = create_sets(4);
    for (int i = 0; i < 4; ++i) {
        eqx_eclass_id_t id = eqx_unionfind_make_set(uf);
        REQUIRE(eqx_unionfind_find(uf, id) == id);
        REQUIRE(eqx_unionfind_num_sets(uf) == (size_t)(i + 1));
    }
    eqx_unionfind_destroy(uf);
}

TEST_CASE("unionfind union self is idempotent", "[unionfind-extensive]") {
    eqx_unionfind_t *uf = create_sets(4);
    eqx_eclass_id_t a = eqx_unionfind_make_set(uf);

    eqx_eclass_id_t rep1 = eqx_unionfind_union(uf, a, a);
    eqx_eclass_id_t rep2 = eqx_unionfind_union(uf, a, a);
    REQUIRE(rep1 == rep2);
    REQUIRE(eqx_unionfind_num_sets(uf) == 1);

    eqx_unionfind_destroy(uf);
}

TEST_CASE("unionfind union is symmetric in effect", "[unionfind-extensive]") {
    eqx_unionfind_t *uf = create_sets(4);
    eqx_eclass_id_t a = eqx_unionfind_make_set(uf);
    eqx_eclass_id_t b = eqx_unionfind_make_set(uf);

    eqx_eclass_id_t ab = eqx_unionfind_union(uf, a, b);
    eqx_eclass_id_t ba = eqx_unionfind_union(uf, b, a);

    REQUIRE(eqx_unionfind_find(uf, a) == eqx_unionfind_find(uf, b));
    REQUIRE(eqx_unionfind_find(uf, ab) == eqx_unionfind_find(uf, ba));

    eqx_unionfind_destroy(uf);
}

TEST_CASE("unionfind union chains are associative", "[unionfind-extensive]") {
    eqx_unionfind_t *uf = create_sets(32);
    const int n = 16;
    eqx_eclass_id_t ids[n];

    for (int i = 0; i < n; ++i) ids[i] = eqx_unionfind_make_set(uf);

    for (int i = 1; i < n; ++i) {
        REQUIRE(eqx_unionfind_union(uf, ids[i - 1], ids[i]) != EQX_ECLASS_ID_INVALID);
    }

    for (int i = 0; i < n; ++i) {
        REQUIRE(eqx_unionfind_find(uf, ids[i]) == eqx_unionfind_find(uf, ids[0]));
    }
    REQUIRE(eqx_unionfind_equiv(uf, ids[0], ids[n - 1]));

    eqx_unionfind_destroy(uf);
}

TEST_CASE("unionfind handles many random-like unions deterministically", "[unionfind-extensive]") {
    eqx_unionfind_t *uf = create_sets(16);
    const int n = 20;
    eqx_eclass_id_t ids[n];

    for (int i = 0; i < n; ++i) ids[i] = eqx_unionfind_make_set(uf);

    REQUIRE(eqx_unionfind_union(uf, ids[0], ids[5]) != EQX_ECLASS_ID_INVALID);
    REQUIRE(eqx_unionfind_union(uf, ids[1], ids[6]) != EQX_ECLASS_ID_INVALID);
    REQUIRE(eqx_unionfind_union(uf, ids[2], ids[7]) != EQX_ECLASS_ID_INVALID);
    REQUIRE(eqx_unionfind_union(uf, ids[5], ids[6]) != EQX_ECLASS_ID_INVALID);
    REQUIRE(eqx_unionfind_union(uf, ids[8], ids[9]) != EQX_ECLASS_ID_INVALID);

    REQUIRE(eqx_unionfind_equiv(uf, ids[0], ids[1]));
    REQUIRE_FALSE(eqx_unionfind_equiv(uf, ids[0], ids[8]));
    REQUIRE(eqx_unionfind_equiv(uf, ids[0], ids[1]));

    eqx_unionfind_destroy(uf);
}

TEST_CASE("unionfind find is stable after repeated lookups", "[unionfind-extensive]") {
    eqx_unionfind_t *uf = create_sets(8);
    eqx_eclass_id_t a = eqx_unionfind_make_set(uf);
    eqx_eclass_id_t b = eqx_unionfind_make_set(uf);
    eqx_eclass_id_t c = eqx_unionfind_make_set(uf);

    eqx_unionfind_union(uf, a, b);
    eqx_unionfind_union(uf, b, c);

    eqx_eclass_id_t rep1 = eqx_unionfind_find(uf, a);
    eqx_eclass_id_t rep2 = eqx_unionfind_find(uf, a);
    REQUIRE(rep1 == rep2);

    REQUIRE(eqx_unionfind_find(uf, c) == rep1);

    eqx_unionfind_destroy(uf);
}

TEST_CASE("unionfind returns invalid for out-of-range find", "[unionfind-extensive]") {
    eqx_unionfind_t *uf = create_sets(4);
    REQUIRE(eqx_unionfind_find(uf, 9) == EQX_ECLASS_ID_INVALID);
    REQUIRE_FALSE(eqx_unionfind_equiv(uf, 0, 9));
    REQUIRE(eqx_unionfind_union(uf, 0, 99) == EQX_ECLASS_ID_INVALID);

    eqx_unionfind_destroy(uf);
}

TEST_CASE("unionfind set_count tracks unions", "[unionfind-extensive]") {
    eqx_unionfind_t *uf = create_sets(64);
    eqx_eclass_id_t ids[6];

    for (size_t i = 0; i < 6; ++i) ids[i] = eqx_unionfind_make_set(uf);
    REQUIRE(eqx_unionfind_num_sets(uf) == 6);

    eqx_unionfind_union(uf, ids[0], ids[1]);
    eqx_unionfind_union(uf, ids[2], ids[3]);
    REQUIRE(eqx_unionfind_num_sets(uf) == 4);

    eqx_unionfind_union(uf, ids[1], ids[2]);
    REQUIRE(eqx_unionfind_num_sets(uf) == 3);

    eqx_unionfind_destroy(uf);
}

TEST_CASE("unionfind unions already-equal elements efficiently", "[unionfind-extensive]") {
    eqx_unionfind_t *uf = create_sets(8);
    eqx_eclass_id_t a = eqx_unionfind_make_set(uf);
    eqx_eclass_id_t b = eqx_unionfind_make_set(uf);
    eqx_unionfind_union(uf, a, b);

    eqx_eclass_id_t rep_before = eqx_unionfind_find(uf, a);
    eqx_eclass_id_t rep_after = eqx_unionfind_union(uf, a, b);
    REQUIRE(rep_after == rep_before);
    REQUIRE(eqx_unionfind_num_sets(uf) == 1);

    eqx_unionfind_destroy(uf);
}

TEST_CASE("unionfind supports 10k unions for stability", "[unionfind-extensive]") {
    const size_t n = 10000;
    eqx_unionfind_t *uf = create_sets(16);
    for (size_t i = 0; i < n; ++i) {
        REQUIRE(eqx_unionfind_make_set(uf) == i);
    }

    for (size_t i = 1; i < n; ++i) {
        REQUIRE(eqx_unionfind_union(uf, i - 1, i) != EQX_ECLASS_ID_INVALID);
    }

    REQUIRE(eqx_unionfind_num_sets(uf) == 1);
    REQUIRE(eqx_unionfind_find(uf, 0) == eqx_unionfind_find(uf, n - 1));

    eqx_unionfind_destroy(uf);
}

TEST_CASE("unionfind stress with repeated redundant unions", "[unionfind-extensive]") {
    eqx_unionfind_t *uf = create_sets(16);
    eqx_eclass_id_t ids[10];
    for (size_t i = 0; i < 10; ++i) ids[i] = eqx_unionfind_make_set(uf);

    for (size_t rep = 0; rep < 5; ++rep) {
        for (size_t i = 0; i < 3; ++i) {
            REQUIRE(eqx_unionfind_union(uf, ids[i], ids[i + 1]) != EQX_ECLASS_ID_INVALID);
            REQUIRE(eqx_unionfind_union(uf, ids[i], ids[i + 1]) != EQX_ECLASS_ID_INVALID);
        }
    }

    REQUIRE(eqx_unionfind_num_sets(uf) == 7);
    REQUIRE(eqx_unionfind_equiv(uf, ids[0], ids[2]));

    eqx_unionfind_destroy(uf);
}
