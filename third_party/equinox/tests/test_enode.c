/* tests/test_enode.c */
#include <catch2/catch_test_macros.hpp>
#include "equinox/enode.h"
#include "equinox/unionfind.h"

TEST_CASE("enode creation and basic operations", "[enode]") {
    SECTION("create constant node") {
        eqx_enode_t *node = eqx_enode_create(42, 0, NULL);
        REQUIRE(node != NULL);
        REQUIRE(eqx_enode_get_operator(node) == 42);
        REQUIRE(eqx_enode_get_arity(node) == 0);
        eqx_enode_destroy(node);
    }
    
    SECTION("create node with children") {
        eqx_eclass_id_t children[] = {1, 2, 3};
        eqx_enode_t *node = eqx_enode_create(100, 3, children);
        REQUIRE(node != NULL);
        REQUIRE(eqx_enode_get_operator(node) == 100);
        REQUIRE(eqx_enode_get_arity(node) == 3);
        REQUIRE(eqx_enode_get_child(node, 0) == 1);
        REQUIRE(eqx_enode_get_child(node, 1) == 2);
        REQUIRE(eqx_enode_get_child(node, 2) == 3);
        eqx_enode_destroy(node);
    }
}

TEST_CASE("enode hashing", "[enode]") {
    SECTION("same structure produces same hash") {
        eqx_eclass_id_t children1[] = {1, 2};
        eqx_eclass_id_t children2[] = {1, 2};
        
        eqx_enode_t *node1 = eqx_enode_create(50, 2, children1);
        eqx_enode_t *node2 = eqx_enode_create(50, 2, children2);
        
        REQUIRE(eqx_enode_hash(node1) == eqx_enode_hash(node2));
        
        eqx_enode_destroy(node1);
        eqx_enode_destroy(node2);
    }
    
    SECTION("different operators produce different hashes") {
        eqx_eclass_id_t children[] = {1, 2};
        
        eqx_enode_t *node1 = eqx_enode_create(50, 2, children);
        eqx_enode_t *node2 = eqx_enode_create(51, 2, children);
        
        REQUIRE(eqx_enode_hash(node1) != eqx_enode_hash(node2));
        
        eqx_enode_destroy(node1);
        eqx_enode_destroy(node2);
    }
    
    SECTION("different children produce different hashes") {
        eqx_eclass_id_t children1[] = {1, 2};
        eqx_eclass_id_t children2[] = {1, 3};
        
        eqx_enode_t *node1 = eqx_enode_create(50, 2, children1);
        eqx_enode_t *node2 = eqx_enode_create(50, 2, children2);
        
        REQUIRE(eqx_enode_hash(node1) != eqx_enode_hash(node2));
        
        eqx_enode_destroy(node1);
        eqx_enode_destroy(node2);
    }
}

TEST_CASE("enode equality", "[enode]") {
    SECTION("identical nodes are equal") {
        eqx_eclass_id_t children1[] = {1, 2, 3};
        eqx_eclass_id_t children2[] = {1, 2, 3};
        
        eqx_enode_t *node1 = eqx_enode_create(100, 3, children1);
        eqx_enode_t *node2 = eqx_enode_create(100, 3, children2);
        
        REQUIRE(eqx_enode_equal(node1, node2));
        
        eqx_enode_destroy(node1);
        eqx_enode_destroy(node2);
    }
    
    SECTION("different operators are not equal") {
        eqx_eclass_id_t children[] = {1, 2};
        
        eqx_enode_t *node1 = eqx_enode_create(100, 2, children);
        eqx_enode_t *node2 = eqx_enode_create(101, 2, children);
        
        REQUIRE_FALSE(eqx_enode_equal(node1, node2));
        
        eqx_enode_destroy(node1);
        eqx_enode_destroy(node2);
    }
    
    SECTION("different arity are not equal") {
        eqx_eclass_id_t children1[] = {1, 2};
        eqx_eclass_id_t children2[] = {1, 2, 3};
        
        eqx_enode_t *node1 = eqx_enode_create(100, 2, children1);
        eqx_enode_t *node2 = eqx_enode_create(100, 3, children2);
        
        REQUIRE_FALSE(eqx_enode_equal(node1, node2));
        
        eqx_enode_destroy(node1);
        eqx_enode_destroy(node2);
    }
}

TEST_CASE("enode canonicalization", "[enode]") {
    eqx_unionfind_t *uf = eqx_unionfind_create(10);
    REQUIRE(uf != NULL);
    
    SECTION("canonicalize updates children to representatives") {
        eqx_eclass_id_t id1 = eqx_unionfind_make_set(uf);
        eqx_eclass_id_t id2 = eqx_unionfind_make_set(uf);
        eqx_eclass_id_t id3 = eqx_unionfind_make_set(uf);
        
        eqx_unionfind_union(uf, id1, id2);
        
        eqx_eclass_id_t children[] = {id2, id3};
        eqx_enode_t *node = eqx_enode_create(50, 2, children);
        
        eqx_enode_canonicalize(node, uf);
        
        eqx_eclass_id_t rep1 = eqx_unionfind_find(uf, id1);
        REQUIRE(eqx_enode_get_child(node, 0) == rep1);
        REQUIRE(eqx_enode_get_child(node, 1) == id3);
        
        eqx_enode_destroy(node);
    }
    
    eqx_unionfind_destroy(uf);
}

TEST_CASE("enode clone", "[enode]") {
    eqx_eclass_id_t children[] = {10, 20, 30};
    eqx_enode_t *original = eqx_enode_create(99, 3, children);
    
    eqx_enode_t *clone = eqx_enode_clone(original);
    REQUIRE(clone != NULL);
    REQUIRE(clone != original);
    REQUIRE(eqx_enode_equal(original, clone));
    REQUIRE(eqx_enode_get_operator(clone) == 99);
    REQUIRE(eqx_enode_get_arity(clone) == 3);
    
    eqx_enode_destroy(original);
    eqx_enode_destroy(clone);
}
