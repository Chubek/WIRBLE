/* tests/test_unionfind.c */
#include <catch2/catch_test_macros.hpp>
#include "equinox/unionfind.h"

TEST_CASE("unionfind creation and destruction", "[unionfind]") {
    eqx_unionfind_t *uf = eqx_unionfind_create(10);
    REQUIRE(uf != NULL);
    eqx_unionfind_destroy(uf);
}

TEST_CASE("unionfind make_set", "[unionfind]") {
    eqx_unionfind_t *uf = eqx_unionfind_create(5);
    
    SECTION("create multiple sets") {
        eqx_eclass_id_t id1 = eqx_unionfind_make_set(uf);
        eqx_eclass_id_t id2 = eqx_unionfind_make_set(uf);
        eqx_eclass_id_t id3 = eqx_unionfind_make_set(uf);
        
        REQUIRE(id1 != EQX_ECLASS_ID_INVALID);
        REQUIRE(id2 != EQX_ECLASS_ID_INVALID);
        REQUIRE(id3 != EQX_ECLASS_ID_INVALID);
        REQUIRE(id1 != id2);
        REQUIRE(id2 != id3);
        REQUIRE(id1 != id3);
    }
    
    SECTION("each set is its own representative initially") {
        eqx_eclass_id_t id = eqx_unionfind_make_set(uf);
        REQUIRE(eqx_unionfind_find(uf, id) == id);
    }
    
    eqx_unionfind_destroy(uf);
}

TEST_CASE("unionfind find", "[unionfind]") {
    eqx_unionfind_t *uf = eqx_unionfind_create(10);
    
    eqx_eclass_id_t id1 = eqx_unionfind_make_set(uf);
    eqx_eclass_id_t id2 = eqx_unionfind_make_set(uf);
    
    SECTION("find returns self for singleton sets") {
        REQUIRE(eqx_unionfind_find(uf, id1) == id1);
        REQUIRE(eqx_unionfind_find(uf, id2) == id2);
    }
    
    SECTION("find is idempotent") {
        eqx_eclass_id_t rep1 = eqx_unionfind_find(uf, id1);
        eqx_eclass_id_t rep2 = eqx_unionfind_find(uf, id1);
        REQUIRE(rep1 == rep2);
    }
    
    eqx_unionfind_destroy(uf);
}

TEST_CASE("unionfind union", "[unionfind]") {
    eqx_unionfind_t *uf = eqx_unionfind_create(10);
    
    eqx_eclass_id_t id1 = eqx_unionfind_make_set(uf);
    eqx_eclass_id_t id2 = eqx_unionfind_make_set(uf);
    eqx_eclass_id_t id3 = eqx_unionfind_make_set(uf);
    
    SECTION("union merges two sets") {
        eqx_eclass_id_t rep = eqx_unionfind_union(uf, id1, id2);
        REQUIRE(rep != EQX_ECLASS_ID_INVALID);
        REQUIRE(eqx_unionfind_find(uf, id1) == eqx_unionfind_find(uf, id2));
    }
    
    SECTION("union is transitive") {
        eqx_unionfind_union(uf, id1, id2);
        eqx_unionfind_union(uf, id2, id3);
        
        REQUIRE(eqx_unionfind_find(uf, id1) == eqx_unionfind_find(uf, id3));
    }
    
    SECTION("union of same set is idempotent") {
        eqx_eclass_id_t rep1 = eqx_unionfind_union(uf, id1, id2);
        eqx_eclass_id_t rep2 = eqx_unionfind_union(uf, id1, id2);
        REQUIRE(rep1 == rep2);
    }
    
    eqx_unionfind_destroy(uf);
}

TEST_CASE("unionfind equivalent", "[unionfind]") {
    eqx_unionfind_t *uf = eqx_unionfind_create(10);
    
    eqx_eclass_id_t id1 = eqx_unionfind_make_set(uf);
    eqx_eclass_id_t id2 = eqx_unionfind_make_set(uf);
    eqx_eclass_id_t id3 = eqx_unionfind_make_set(uf);
    
    SECTION("sets are not equivalent initially") {
        REQUIRE_FALSE(eqx_unionfind_equivalent(uf, id1, id2));
        REQUIRE_FALSE(eqx_unionfind_equivalent(uf, id2, id3));
    }
    
    SECTION("sets are equivalent after union") {
        eqx_unionfind_union(uf, id1, id2);
        REQUIRE(eqx_unionfind_equivalent(uf, id1, id2));
        REQUIRE_FALSE(eqx_unionfind_equivalent(uf, id1, id3));
    }
    
    SECTION("equivalence is reflexive") {
        REQUIRE(eqx_unionfind_equivalent(uf, id1, id1));
    }
    
    SECTION("equivalence is symmetric") {
        eqx_unionfind_union(uf, id1, id2);
        REQUIRE(eqx_unionfind_equivalent(uf, id1, id2));
        REQUIRE(eqx_unionfind_equivalent(uf, id2, id1));
    }
    
    eqx_unionfind_destroy(uf);
}

TEST_CASE("unionfind count_sets", "[unionfind]") {
    eqx_unionfind_t *uf = eqx_unionfind_create(10);
    
    SECTION("initially zero sets") {
        REQUIRE(eqx_unionfind_count_sets(uf) == 0);
    }
    
    SECTION("count increases with make_set") {
        eqx_unionfind_make_set(uf);
        REQUIRE(eqx_unionfind_count_sets(uf) == 1);
        
        eqx_unionfind_make_set(uf);
        REQUIRE(eqx_unionfind_count_sets(uf) == 2);
        
        eqx_unionfind_make_set(uf);
        REQUIRE(eqx_unionfind_count_sets(uf) == 3);
    }
    
    SECTION("count decreases with union") {
        eqx_eclass_id_t id1 = eqx_unionfind_make_set(uf);
        eqx_eclass_id_t id2 = eqx_unionfind_make_set(uf);
        eqx_eclass_id_t id3 = eqx_unionfind_make_set(uf);
        
        REQUIRE(eqx_unionfind_count_sets(uf) == 3);
        
        eqx_unionfind_union(uf, id1, id2);
        REQUIRE(eqx_unionfind_count_sets(uf) == 2);
        
        eqx_unionfind_union(uf, id2, id3);
        REQUIRE(eqx_unionfind_count_sets(uf) == 1);
    }
    
    eqx_unionfind_destroy(uf);
}

TEST_CASE("unionfind path compression", "[unionfind]") {
    eqx_unionfind_t *uf = eqx_unionfind_create(10);
    
    eqx_eclass_id_t id1 = eqx_unionfind_make_set(uf);
    eqx_eclass_id_t id2 = eqx_unionfind_make_set(uf);
    eqx_eclass_id_t id3 = eqx_unionfind_make_set(uf);
    eqx_eclass_id_t id4 = eqx_unionfind_make_set(uf);
    
    eqx_unionfind_union(uf, id1, id2);
    eqx_unionfind_union(uf, id2, id3);
    eqx_unionfind_union(uf, id3, id4);
    
    eqx_eclass_id_t rep = eqx_unionfind_find(uf, id4);
    
    SECTION("all elements point to same representative") {
        REQUIRE(eqx_unionfind_find(uf, id1) == rep);
        REQUIRE(eqx_unionfind_find(uf, id2) == rep);
        REQUIRE(eqx_unionfind_find(uf, id3) == rep);
        REQUIRE(eqx_unionfind_find(uf, id4) == rep);
    }
    
    eqx_unionfind_destroy(uf);
}
