#include "equinox/egraph.h"
#include "equinox/enode.h"
#include "equinox/eclass.h"
#include "equinox/unionfind.h"
#include "equinox/hashcons.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

/* Default configuration values */
static const eqx_egraph_config_t DEFAULT_CONFIG = {
    .initial_capacity = 1024,
    .enable_explanations = false,
    .max_iterations = 1000,
    .node_limit = 100000
};

/* E-graph structure */
struct eqx_egraph {
    eqx_unionfind_t *uf;
    eqx_hashcons_t *hashcons;
    eqx_egraph_config_t config;
    size_t num_eclasses;
    eqx_eclass_t **eclasses;
    size_t eclasses_capacity;
    bool dirty;
    size_t pending_count;
    size_t union_count;
};

/* Iterator structure */
struct eqx_egraph_iter {
    eqx_egraph_t *egraph;
    size_t current;
};

/* Configuration */
eqx_egraph_config_t eqx_egraph_config_default(void) {
    return DEFAULT_CONFIG;
}

eqx_egraph_t *eqx_egraph_create(const eqx_egraph_config_t *config) {
    eqx_egraph_t *eg = malloc(sizeof(eqx_egraph_t));
    if (!eg) return NULL;

    eg->config = config ? *config : DEFAULT_CONFIG;
    eg->num_eclasses = 0;
    eg->dirty = false;
    eg->pending_count = 0;
    eg->union_count = 0;

    eg->uf = eqx_unionfind_create(eg->config.initial_capacity);
    if (!eg->uf) {
        free(eg);
        return NULL;
    }

    eg->hashcons = eqx_hashcons_create(eg->config.initial_capacity,
                                       0.75);
    if (!eg->hashcons) {
        eqx_unionfind_destroy(eg->uf);
        free(eg);
        return NULL;
    }

    eg->eclasses_capacity = eg->config.initial_capacity;
    eg->eclasses = calloc(eg->eclasses_capacity, sizeof(eqx_eclass_t *));
    if (!eg->eclasses) {
        eqx_hashcons_destroy(eg->hashcons);
        eqx_unionfind_destroy(eg->uf);
        free(eg);
        return NULL;
    }

    return eg;
}

void eqx_egraph_destroy(eqx_egraph_t *eg) {
    if (!eg) return;

    for (size_t i = 0; i < eg->num_eclasses; i++) {
        if (eg->eclasses[i]) {
            eqx_eclass_destroy(eg->eclasses[i]);
        }
    }
    free(eg->eclasses);

    eqx_hashcons_destroy(eg->hashcons);
    eqx_unionfind_destroy(eg->uf);
    free(eg);
}

/* Term operations */
eqx_eclass_id_t eqx_egraph_add(eqx_egraph_t *eg, eqx_operator_t op,
                                size_t arity, const eqx_eclass_id_t *children) {
    assert(eg);

    eqx_eclass_id_t *canon_children = NULL;
    if (arity > 0) {
        canon_children = malloc(arity * sizeof(eqx_eclass_id_t));
        if (!canon_children) return EQX_ECLASS_ID_INVALID;

        for (size_t i = 0; i < arity; i++) {
            canon_children[i] = eqx_unionfind_find(eg->uf, children[i]);
        }
    }

    eqx_enode_t *temp = eqx_enode_create(op, arity, canon_children);
    free(canon_children);
    if (!temp) return EQX_ECLASS_ID_INVALID;

    eqx_eclass_id_t existing_id = EQX_ECLASS_ID_INVALID;
    if (eqx_hashcons_lookup(eg->hashcons, temp, &existing_id)) {
        eqx_enode_destroy(temp);
        return existing_id;
    }

    eqx_eclass_id_t new_id = eqx_unionfind_make_set(eg->uf);
    if (new_id == EQX_ECLASS_ID_INVALID) {
        eqx_enode_destroy(temp);
        return EQX_ECLASS_ID_INVALID;
    }

    if (new_id >= eg->eclasses_capacity) {
        size_t new_cap = eg->eclasses_capacity ? (eg->eclasses_capacity * 2) : 16;
        eqx_eclass_t **new_eclasses = realloc(eg->eclasses, new_cap * sizeof(eqx_eclass_t *));
        if (!new_eclasses) {
            eqx_enode_destroy(temp);
            return EQX_ECLASS_ID_INVALID;
        }
        memset(new_eclasses + eg->eclasses_capacity, 0,
               (new_cap - eg->eclasses_capacity) * sizeof(eqx_eclass_t *));
        eg->eclasses = new_eclasses;
        eg->eclasses_capacity = new_cap;
    }

    eqx_eclass_t *eclass = eqx_eclass_create(new_id);
    if (!eclass) {
        eqx_enode_destroy(temp);
        return EQX_ECLASS_ID_INVALID;
    }

    eg->eclasses[new_id] = eclass;
    eg->num_eclasses++;

    eqx_enode_set_eclass(temp, new_id);
    eqx_eclass_add_node(eclass, temp);

    if (!eqx_hashcons_insert(eg->hashcons, temp, new_id)) {
        eqx_enode_destroy(temp);
        eqx_eclass_destroy(eclass);
        eg->eclasses[new_id] = NULL;
        return EQX_ECLASS_ID_INVALID;
    }

    for (size_t i = 0; i < arity; i++) {
        eqx_eclass_id_t child_id = eqx_enode_get_child(temp, i);
        eqx_eclass_id_t canon_child = eqx_unionfind_find(eg->uf, child_id);
        if (canon_child < eg->num_eclasses && eg->eclasses[canon_child]) {
            eqx_eclass_add_parent(eg->eclasses[canon_child], temp);
        }
    }

    return new_id;
}

bool eqx_egraph_union(eqx_egraph_t *eg, eqx_eclass_id_t id1, eqx_eclass_id_t id2) {
    assert(eg);

    id1 = eqx_unionfind_find(eg->uf, id1);
    id2 = eqx_unionfind_find(eg->uf, id2);

    if (id1 == id2) return true;

    eqx_eclass_id_t new_id = eqx_unionfind_union(eg->uf, id1, id2);
    if (new_id == EQX_ECLASS_ID_INVALID) return false;
    ++eg->union_count;
    eqx_eclass_id_t old_id = (new_id == id1) ? id2 : id1;

    if (new_id < eg->num_eclasses && old_id < eg->num_eclasses &&
        eg->eclasses[new_id] && eg->eclasses[old_id]) {
        eqx_eclass_merge_into(eg->eclasses[old_id], eg->eclasses[new_id]);
    }

    eg->dirty = true;
    eg->pending_count++;

    return true;
}

eqx_eclass_id_t eqx_egraph_find(eqx_egraph_t *eg, eqx_eclass_id_t id) {
    assert(eg);
    return eqx_unionfind_find(eg->uf, id);
}

bool eqx_egraph_equiv(eqx_egraph_t *eg, eqx_eclass_id_t id1, eqx_eclass_id_t id2) {
    assert(eg);
    return eqx_unionfind_equiv(eg->uf, id1, id2);
}

bool eqx_egraph_rebuild(eqx_egraph_t *eg) {
    assert(eg);

    if (!eg->dirty) return true;

    size_t iterations = 0;
    while (eg->pending_count > 0 && iterations < eg->config.max_iterations) {
        eg->pending_count = 0;
        eqx_hashcons_t *rebuilt = eqx_hashcons_create(eg->config.initial_capacity,
                                                      0.75);
        if (!rebuilt) return false;

        for (size_t i = 0; i < eg->num_eclasses; i++) {
            eqx_eclass_t *eclass = eg->eclasses[i];
            if (!eclass) continue;

            eqx_eclass_id_t canon_id = eqx_unionfind_find(eg->uf, i);
            if (canon_id != i) continue;

            for (eqx_enode_t *node = eqx_eclass_get_nodes(eclass); node != NULL; node = node->next_in_class) {
                eqx_enode_canonicalize(node, eg->uf);
                eqx_eclass_id_t node_class = eqx_unionfind_find(eg->uf,
                                                                eqx_enode_get_eclass(node));
                node->eclass = node_class;

                eqx_eclass_id_t existing_id = EQX_ECLASS_ID_INVALID;
                if (eqx_hashcons_lookup(rebuilt, node, &existing_id)) {
                    eqx_egraph_union(eg, node_class, existing_id);
                    continue;
                }

                if (!eqx_hashcons_insert(rebuilt, node, node_class)) {
                    eqx_hashcons_destroy(rebuilt);
                    return false;
                }
            }
        }

        eqx_hashcons_destroy(eg->hashcons);
        eg->hashcons = rebuilt;

        iterations++;
    }

    eg->dirty = false;
    return iterations < eg->config.max_iterations;
}

size_t eqx_egraph_num_eclasses(const eqx_egraph_t *eg) {
    assert(eg);
    return eg->num_eclasses;
}

eqx_eclass_t *eqx_egraph_get_eclass(eqx_egraph_t *eg, eqx_eclass_id_t id) {
    assert(eg);
    id = eqx_unionfind_find(eg->uf, id);
    if (id >= eg->num_eclasses) return NULL;
    return eg->eclasses[id];
}

void eqx_egraph_get_stats(const eqx_egraph_t *eg, eqx_egraph_stats_t *out_stats) {
    assert(eg);
    assert(out_stats);

    out_stats->num_eclasses = eg->num_eclasses;
    out_stats->num_enodes = eqx_hashcons_size(eg->hashcons);
    out_stats->num_unions = eg->union_count;
    out_stats->hashcons_size = eqx_hashcons_size(eg->hashcons);
}

/* Iterator */
eqx_egraph_iter_t eqx_egraph_iter_begin(eqx_egraph_t *eg) {
    assert(eg);
    struct eqx_egraph_iter* iter = malloc(sizeof(struct eqx_egraph_iter));
    if (!iter) return NULL;
    iter->egraph = eg;
    iter->current = 0;
    return iter;
}

void eqx_egraph_iter_end(eqx_egraph_iter_t iter) {
    free(iter);
}

bool eqx_egraph_iter_has_next(const eqx_egraph_iter_t iter) {
    assert(iter);
    for (size_t i = iter->current; i < iter->egraph->num_eclasses; i++) {
        eqx_eclass_id_t canon = eqx_unionfind_find(iter->egraph->uf, i);
        if (canon == i && iter->egraph->eclasses[i]) {
            return true;
        }
    }
    return false;
}

eqx_eclass_id_t eqx_egraph_iter_next(eqx_egraph_iter_t iter) {
    assert(iter);

    while (iter->current < iter->egraph->num_eclasses) {
        size_t i = iter->current++;
        eqx_eclass_id_t canon = eqx_unionfind_find(iter->egraph->uf, i);
        if (canon == i && iter->egraph->eclasses[i]) {
            return (eqx_eclass_id_t)i;
        }
    }
    return EQX_ECLASS_ID_INVALID;
}

void eqx_egraph_print(const eqx_egraph_t *eg, FILE *out) {
    assert(eg);
    if (!out) out = stdout;

    fprintf(out, "E-graph: %zu e-classes, %zu e-nodes\n",
            eg->num_eclasses, eqx_hashcons_size(eg->hashcons));

    for (size_t i = 0; i < eg->num_eclasses; i++) {
        eqx_eclass_id_t canon = eqx_unionfind_find(eg->uf, i);
        if (canon == i && eg->eclasses[i]) {
            eqx_eclass_print(eg->eclasses[i], out);
        }
    }
}
