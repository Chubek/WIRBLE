#include "equinox/unionfind.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Creation and destruction */
eqx_unionfind_t *eqx_unionfind_create(size_t initial_capacity) {
    eqx_unionfind_t *uf = malloc(sizeof(eqx_unionfind_t));
    if (!uf) return NULL;

    uf->capacity = initial_capacity > 0 ? initial_capacity : 16;
    uf->parent = malloc(uf->capacity * sizeof(*uf->parent));
    uf->rank = malloc(uf->capacity * sizeof(*uf->rank));
    if (!uf->parent || !uf->rank) {
        free(uf->parent);
        free(uf->rank);
        free(uf);
        return NULL;
    }

    uf->size = 0;
    uf->set_count = 0;

    return uf;
}

void eqx_unionfind_destroy(eqx_unionfind_t *uf) {
    if (!uf) return;
    free(uf->parent);
    free(uf->rank);
    free(uf);
}

/* Set operations */
eqx_eclass_id_t eqx_unionfind_make_set(eqx_unionfind_t *uf) {
    assert(uf);

    /* Expand if needed */
    if (uf->size >= uf->capacity) {
        size_t new_capacity = uf->capacity * 2;
        uf_id_t *new_parent = realloc(uf->parent, new_capacity * sizeof(*uf->parent));
        uint32_t *new_rank = realloc(uf->rank, new_capacity * sizeof(*uf->rank));
        if (!new_parent || !new_rank) {
            free(new_parent && new_parent != uf->parent ? new_parent : NULL);
            free(new_rank && new_rank != uf->rank ? new_rank : NULL);
            return EQX_ECLASS_ID_INVALID;
        }
        uf->parent = new_parent;
        uf->rank = new_rank;
        uf->capacity = new_capacity;
    }

    eqx_eclass_id_t new_id = uf->size++;
    uf->parent[new_id] = new_id;
    uf->rank[new_id] = 0;
    uf->set_count++;

    return new_id;
}

eqx_eclass_id_t eqx_unionfind_find(eqx_unionfind_t *uf, eqx_eclass_id_t id) {
    assert(uf);
    if (id >= uf->size) return EQX_ECLASS_ID_INVALID;

    /* Find root with path compression */
    if (uf->parent[id] != id) {
        uf->parent[id] = eqx_unionfind_find(uf, uf->parent[id]);
    }

    return uf->parent[id];
}

eqx_eclass_id_t eqx_unionfind_union(eqx_unionfind_t *uf, eqx_eclass_id_t id1, eqx_eclass_id_t id2) {
    assert(uf);
    if (id1 >= uf->size || id2 >= uf->size) return EQX_ECLASS_ID_INVALID;

    eqx_eclass_id_t root1 = eqx_unionfind_find(uf, id1);
    eqx_eclass_id_t root2 = eqx_unionfind_find(uf, id2);

    if (root1 == root2) return root1;

    /* Union by rank */
    if (uf->rank[root1] < uf->rank[root2]) {
        uf->parent[root1] = root2;
        uf->set_count--;
        return root2;
    } else if (uf->rank[root1] > uf->rank[root2]) {
        uf->parent[root2] = root1;
        uf->set_count--;
        return root1;
    } else {
        uf->parent[root2] = root1;
        uf->rank[root1]++;
        uf->set_count--;
        return root1;
    }
}

bool eqx_unionfind_equiv(eqx_unionfind_t *uf, eqx_eclass_id_t id1, eqx_eclass_id_t id2) {
    assert(uf);
    if (id1 >= uf->size || id2 >= uf->size) return false;
    return eqx_unionfind_find(uf, id1) == eqx_unionfind_find(uf, id2);
}

/* Accessors */
size_t eqx_unionfind_size(const eqx_unionfind_t *uf) {
    assert(uf);
    return uf->size;
}

size_t eqx_unionfind_num_sets(const eqx_unionfind_t *uf) {
    assert(uf);
    return uf->set_count;
}
