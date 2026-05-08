/* tests/test_egraph.c */
#include <catch2/catch_test_macros.hpp>
#include "equinox/egraph.h"

TEST_CASE("egraph creation and destruction", "[egraph]") {
    eqx_egraph_config_t config = eqx_egraph_config_default();
    eqx_egraph_t *egraph = eqx_egraph_create(&config);
    REQUIRE(egraph != NULL);
    eqx_egraph_destroy(egraph);
}

TEST_CASE("egraph add constants", "[egraph]") {
    eqx_egraph_config_t config = eqx_egraph_config_default();
    eqx_egraph_t *egraph = eqx_egraph_create(&config);
    
    SECTION("add single constant") {
        eqx_eclass_id_t id = eqx_egraph_add(egraph, 42, 0, NULL);
        REQUIRE(id != EQX_ECLASS_ID_INVALID);
    }
    
    SECTION("add multiple constants") {
        eqx_eclass_id_t id1 = eqx_egraph_add(egraph, 1, 0, NULL);
        eqx_eclass_id_t id2 = eqx_egraph_add(egraph, 2, 0, NULL);
        eqx_eclass_id_t id3 = eqx_egraph_add(egraph, 3, 0, NULL);
        
        REQUIRE(id1 != EQX_ECLASS_ID_INVALID);
        REQUIRE(id2 != EQX_ECLASS_ID_INVALID);
        REQUIRE(id3 != EQX_ECLASS_ID_INVALID);
        REQUIRE(id1 != id2);
        REQUIRE(id2 != id3);
    }
    
    SECTION("same constant added twice returns same e-class") {
        eqx_eclass_id_t id1 = eqx_egraph_add(egraph, 42, 0, NULL);
        eqx_eclass_id_t id2 = eqx_egraph_add(egraph, 42, 0, NULL);
        REQUIRE(id1 == id2);
    }
    
    eqx_egraph_destroy(egraph);
}

TEST_CASE("egraph add applications", "[egraph]") {
    eqx_egraph_config_t config = eqx_egraph_config_default();
    eqx_egraph_t *egraph = eqx_egraph_create(&config);
    
    SECTION("add application with children") {
        eqx_eclass_id_t c1 = eqx_egraph_add(egraph, 1, 0, NULL);
        eqx_eclass_id_t c2 = eqx_egraph_add(egraph, 2, 0, NULL);
        
        eqx_eclass_id_t children[] = {c1, c2};
        eqx_eclass_id_t app = eqx_egraph_add(egraph, 100, 2, children);
        
        REQUIRE(app != EQX_ECLASS_ID_INVALID);
        REQUIRE(app != c1);
        REQUIRE(app != c2);
    }
    
    SECTION("same application returns same e-class") {
        eqx_eclass_id_t c1 = eqx_egraph_add(egraph, 1, 0, NULL);
        eqx_eclass_id_t c2 = eqx_egraph_add(egraph, 2, 0, NULL);
        
        eqx_eclass_id_t children[] = {c1, c2};
        eqx_eclass_id_t app1 = eqx_egraph_add(egraph, 100, 2, children);
        eqx_eclass_id_t app2 = eqx_egraph_add(egraph, 100, 2, children);
        
        REQUIRE(app1 == app2);
    }
    
    eqx_egraph_destroy(egraph);
}

TEST_CASE("egraph union", "[egraph]") {
    eqx_egraph_config_t config = eqx_egraph_config_default();
    eqx_egraph_t *egraph = eqx_egraph_create(&config);
    
    SECTION("union two constants") {
        eqx_eclass_id_t id1 = eqx_egraph_add(egraph, 1, 0, NULL);
        eqx_eclass_id_t id2 = eqx_egraph_add(egraph, 2, 0, NULL);
        
        eqx_eclass_id_t rep = eqx_egraph_union(egraph, id1, id2);
        REQUIRE(rep != EQX_ECLASS_ID_INVALID);
        
        REQUIRE(eqx_egraph_find(egraph, id1) == eqx_egraph_find(egraph, id2));
    }
    
    SECTION("union is idempotent") {
        eqx_eclass_id_t id1 = eqx_egraph_add(egraph, 1, 0, NULL);
        eqx_eclass_id_t id2 = eqx_egraph_add(egraph, 2, 0, NULL);
        
        eqx_eclass_id_t rep1 = eqx_egraph_union(egraph, id1, id2);
        eqx_eclass_id_t rep2 = eqx_egraph_union(egraph, id1, id2);
        
        REQUIRE(rep1 == rep2);
    }
    
    eqx_egraph_destroy(egraph);
}

TEST_CASE("egraph congruence closure", "[egraph]") {
    eqx_egraph_config_t config = eqx_egraph_config_default();
    eqx_egraph_t *egraph = eqx_egraph_create(&config);
    
    SECTION("congruence propagates through applications") {
        // Create: f(a, b) and f(c, d)
        eqx_eclass_id_t a = eqx_egraph_add(egraph, 1, 0, NULL);
        eqx_eclass_id_t b = eqx_egraph_add(egraph, 2, 0, NULL);
        eqx_eclass_id_t c = eqx_egraph_add(egraph, 3, 0, NULL);
        eqx_eclass_id_t d = eqx_egraph_add(egraph, 4, 0, NULL);
        
        eqx_eclass_id_t children1[] = {a, b};
        eqx_eclass_id_t f_ab = eqx_egraph_add(egraph, 100, 2, children1);
        
        eqx_eclass_id_t children2[] = {c, d};
        eqx_eclass_id_t f_cd = eqx_egraph_add(egraph, 100, 2, children2);
        
        // Assert a = c and b = d
        eqx_egraph_union(egraph, a, c);
        eqx_egraph_union(egraph, b, d);
        
        // Rebuild to propagate congruence
        eqx_egraph_rebuild(egraph);
        
        // f(a,b) should equal f(c,d) by congruence
        REQUIRE(eqx_egraph_find(egraph, f_ab) == eqx_egraph_find(egraph, f_cd));
    }
    
    eqx_egraph_destroy(egraph);
}

TEST_CASE("egraph find", "[egraph]") {
    eqx_egraph_config_t config = eqx_egraph_config_default();
    eqx_egraph_t *egraph = eqx_egraph_create(&config);
    
    eqx_eclass_id_t id1 = eqx_egraph_add(egraph, 1, 0, NULL);
    eqx_eclass_id_t id2 = eqx_egraph_add(egraph, 2, 0, NULL);
    
    SECTION("find returns canonical representative") {
        REQUIRE(eqx_egraph_find(egraph, id1) == id1);
        REQUIRE(eqx_egraph_find(egraph, id2) == id2);
    }
    
    SECTION("find returns same representative after union") {
        eqx_egraph_union(egraph, id1, id2);
        REQUIRE(eqx_egraph_find(egraph, id1) == eqx_egraph_find(egraph, id2));
    }
    
    eqx_egraph_destroy(egraph);
}

TEST_CASE("egraph statistics", "[egraph]") {
    eqx_egraph_config_t config = eqx_egraph_config_default();
    eqx_egraph_t *egraph = eqx_egraph_create(&config);
    
    eqx_egraph_stats_t stats;
    
    SECTION("initial statistics") {
        eqx_egraph_get_stats(egraph, &stats);
        REQUIRE(stats.num_enodes == 0);
        REQUIRE(stats.num_eclasses == 0);
        REQUIRE(stats.num_unions == 0);
    }
    
    SECTION("statistics after adding nodes") {
        eqx_egraph_add(egraph, 1, 0, NULL);
        eqx_egraph_add(egraph, 2, 0, NULL);
        
        eqx_egraph_get_stats(egraph, &stats);
        REQUIRE(stats.num_enodes == 2);
        REQUIRE(stats.num_eclasses == 2);
    }
    
    SECTION("statistics after union") {
        eqx_eclass_id_t id1 = eqx_egraph_add(egraph, 1, 0, NULL);
        eqx_eclass_id_t id2 = eqx_egraph_add(egraph, 2, 0, NULL);
        
        eqx_egraph_union(egraph, id1, id2);
        
        eqx_egraph_get_stats(egraph, &stats);
        REQUIRE(stats.num_unions == 1);
    }
    
    eqx_egraph_destroy(egraph);
}

TEST_CASE("egraph iterator", "[egraph]") {
    eqx_egraph_config_t config = eqx_egraph_config_default();
    eqx_egraph_t *egraph = eqx_egraph_create(&config);
    
    eqx_egraph_add(egraph, 1, 0, NULL);
    eqx_egraph_add(egraph, 2, 0, NULL);
    eqx_egraph_add(egraph, 3, 0, NULL);
    
    SECTION("iterate over all e-classes") {
        size_t count = 0;
        eqx_egraph_iter_t iter = eqx_egraph_iter_begin(egraph);
        
        while (eqx_egraph_iter_has_next(iter)) {
            eqx_eclass_id_t id = eqx_egraph_iter_next(iter);
            REQUIRE(id != EQX_ECLASS_ID_INVALID);
            count++;
        }
        
        eqx_egraph_iter_end(iter);
        REQUIRE(count == 3);
    }
    
    eqx_egraph_destroy(egraph);
}
