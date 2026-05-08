/* tests/test_hashcons.c */
#include <catch2/catch_test_macros.hpp>
#include "equinox/hashcons.h"
#include "equinox/enode.h"

TEST_CASE("hashcons creation and destruction", "[hashcons]") {
    eqx_hashcons_t *hc = eqx_hashcons_create(16, 0.75);
    REQUIRE(hc != NULL);
    eqx_hashcons_destroy(hc);
}

TEST_CASE("hashcons insert and lookup", "[hashcons]") {
    eqx_hashcons_t *hc = eqx_hashcons_create(16, 0.75);
    
    SECTION("insert single node") {
        eqx_enode_t *node = eqx_enode_create(42, 0, NULL);
        eqx_eclass_id_t id = 100;
        
        bool inserted = eqx_hashcons_insert(hc, node, id);
        REQUIRE(inserted);
        
        eqx_enode_destroy(node);
    }
    
    SECTION("lookup existing node") {
        eqx_enode_t *node = eqx_enode_create(42, 0, NULL);
        eqx_eclass_id_t id = 100;
        
        eqx_hashcons_insert(hc, node, id);
        
        eqx_eclass_id_t found_id;
        bool found = eqx_hashcons_lookup(hc, node, &found_id);
        REQUIRE(found);
        REQUIRE(found_id == id);
        
        eqx_enode_destroy(node);
    }
    
    SECTION("lookup non-existing node") {
        eqx_enode_t *node = eqx_enode_create(42, 0, NULL);
        
        eqx_eclass_id_t found_id;
        bool found = eqx_hashcons_lookup(hc, node, &found_id);
        REQUIRE_FALSE(found);
        
        eqx_enode_destroy(node);
    }
    
    SECTION("insert duplicate returns false") {
        eqx_enode_t *node = eqx_enode_create(42, 0, NULL);
        eqx_eclass_id_t id = 100;
        
        bool inserted1 = eqx_hashcons_insert(hc, node, id);
        bool inserted2 = eqx_hashcons_insert(hc, node, id);
        
        REQUIRE(inserted1);
        REQUIRE_FALSE(inserted2);
        
        eqx_enode_destroy(node);
    }
    
    eqx_hashcons_destroy(hc);
}

TEST_CASE("hashcons remove", "[hashcons]") {
    eqx_hashcons_t *hc = eqx_hashcons_create(16, 0.75);
    
    SECTION("remove existing node") {
        eqx_enode_t *node = eqx_enode_create(42, 0, NULL);
        eqx_eclass_id_t id = 100;
        
        eqx_hashcons_insert(hc, node, id);
        
        bool removed = eqx_hashcons_remove(hc, node);
        REQUIRE(removed);
        
        eqx_eclass_id_t found_id;
        bool found = eqx_hashcons_lookup(hc, node, &found_id);
        REQUIRE_FALSE(found);
        
        eqx_enode_destroy(node);
    }
    
    SECTION("remove non-existing node") {
        eqx_enode_t *node = eqx_enode_create(42, 0, NULL);
        
        bool removed = eqx_hashcons_remove(hc, node);
        REQUIRE_FALSE(removed);
        
        eqx_enode_destroy(node);
    }
    
    eqx_hashcons_destroy(hc);
}

TEST_CASE("hashcons multiple nodes", "[hashcons]") {
    eqx_hashcons_t *hc = eqx_hashcons_create(16, 0.75);
    
    SECTION("insert and lookup multiple nodes") {
        eqx_enode_t *node1 = eqx_enode_create(1, 0, NULL);
        eqx_enode_t *node2 = eqx_enode_create(2, 0, NULL);
        eqx_enode_t *node3 = eqx_enode_create(3, 0, NULL);
        
        eqx_hashcons_insert(hc, node1, 100);
        eqx_hashcons_insert(hc, node2, 200);
        eqx_hashcons_insert(hc, node3, 300);
        
        eqx_eclass_id_t id1, id2, id3;
        REQUIRE(eqx_hashcons_lookup(hc, node1, &id1));
        REQUIRE(eqx_hashcons_lookup(hc, node2, &id2));
        REQUIRE(eqx_hashcons_lookup(hc, node3, &id3));
        
        REQUIRE(id1 == 100);
        REQUIRE(id2 == 200);
        REQUIRE(id3 == 300);
        
        eqx_enode_destroy(node1);
        eqx_enode_destroy(node2);
        eqx_enode_destroy(node3);
    }
    
    eqx_hashcons_destroy(hc);
}

TEST_CASE("hashcons iterator", "[hashcons]") {
    eqx_hashcons_t *hc = eqx_hashcons_create(16, 0.75);
    
    eqx_enode_t *node1 = eqx_enode_create(1, 0, NULL);
    eqx_enode_t *node2 = eqx_enode_create(2, 0, NULL);
    eqx_enode_t *node3 = eqx_enode_create(3, 0, NULL);
    
    eqx_hashcons_insert(hc, node1, 100);
    eqx_hashcons_insert(hc, node2, 200);
    eqx_hashcons_insert(hc, node3, 300);
    
    SECTION("iterate over all entries") {
        size_t count = 0;
        eqx_hashcons_iter_t iter = eqx_hashcons_iter_begin(hc);
        
        while (eqx_hashcons_iter_has_next(iter)) {
            eqx_enode_t *node;
            eqx_eclass_id_t id;
            eqx_hashcons_iter_next(iter, &node, &id);
            
            REQUIRE(node != NULL);
            REQUIRE(id != EQX_ECLASS_ID_INVALID);
            count++;
        }
        
        eqx_hashcons_iter_end(iter);
        REQUIRE(count == 3);
    }
    
    eqx_enode_destroy(node1);
    eqx_enode_destroy(node2);
    eqx_enode_destroy(node3);
    eqx_hashcons_destroy(hc);
}

TEST_CASE("hashcons resizing", "[hashcons]") {
    eqx_hashcons_t *hc = eqx_hashcons_create(4, 0.75);
    
    SECTION("automatic resize on load factor") {
        // Insert enough nodes to trigger resize
        for (int i = 0; i < 10; i++) {
            eqx_enode_t *node = eqx_enode_create(i, 0, NULL);
            eqx_hashcons_insert(hc, node, i + 100);
            eqx_enode_destroy(node);
        }
        
        // Verify all nodes are still accessible
        for (int i = 0; i < 10; i++) {
            eqx_enode_t *node = eqx_enode_create(i, 0, NULL);
            eqx_eclass_id_t id;
            bool found = eqx_hashcons_lookup(hc, node, &id);
            REQUIRE(found);
            REQUIRE(id == i + 100);
            eqx_enode_destroy(node);
        }
    }
    
    eqx_hashcons_destroy(hc);
}

TEST_CASE("hashcons collision handling", "[hashcons]") {
    eqx_hashcons_t *hc = eqx_hashcons_create(4, 0.75);
    
    SECTION("handle hash collisions") {
        // Create nodes that may collide in small table
        eqx_eclass_id_t children1[] = {1, 2};
        eqx_eclass_id_t children2[] = {3, 4};
        
        eqx_enode_t *node1 = eqx_enode_create(100, 2, children1);
        eqx_enode_t *node2 = eqx_enode_create(100, 2, children2);
        
        eqx_hashcons_insert(hc, node1, 1000);
        eqx_hashcons_insert(hc, node2, 2000);
        
        eqx_eclass_id_t id1, id2;
        REQUIRE(eqx_hashcons_lookup(hc, node1, &id1));
        REQUIRE(eqx_hashcons_lookup(hc, node2, &id2));
        
        REQUIRE(id1 == 1000);
        REQUIRE(id2 == 2000);
        
        eqx_enode_destroy(node1);
        eqx_enode_destroy(node2);
    }
    
    eqx_hashcons_destroy(hc);
}

