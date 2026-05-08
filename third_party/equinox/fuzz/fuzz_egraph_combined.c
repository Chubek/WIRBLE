#include <stdint.h>
#include <stddef.h>
#include "equinox/egraph.h"
#include "equinox/rewrite.h"
#include "equinox/hashcons.h"
#include "equinox/unionfind.h"

static size_t bounded(size_t v, size_t hi) {
    return (hi == 0) ? 0 : (v % hi);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 4) return 0;

    eqx_egraph_t *g = eqx_egraph_create(NULL);
    if (!g) return 0;
    eqx_hashcons_t *hc = eqx_hashcons_create(4, 0.75);
    eqx_unionfind_t *uf = eqx_unionfind_create(32);
    if (!hc || !uf) {
        if (hc) eqx_hashcons_destroy(hc);
        if (uf) eqx_unionfind_destroy(uf);
        eqx_egraph_destroy(g);
        return 0;
    }

    eqx_eclass_id_t ids[1024] = {EQX_ECLASS_ID_INVALID};
    eqx_enode_t *enodes[1024] = {NULL};
    eqx_eclass_id_t id_count = 0;
    eqx_eclass_id_t node_count = 0;
    size_t pos = 0;
    const size_t max_ops = 2048;

    for (size_t iter = 0; iter < max_ops; ++iter) {
        uint8_t op = data[pos % size];
        size_t i = pos % size;

        switch (op % 32) {
            case 0:
                break;
            case 1: {
                uint8_t op_id = data[(i + 1) % size];
                eqx_eclass_id_t a = eqx_egraph_add(g, (uint32_t)op_id, 0, NULL);
                if (a != EQX_ECLASS_ID_INVALID && id_count < 1024) ids[id_count++] = a;
                pos += 1;
                break;
            }
            case 2: {
                uint8_t op_id = data[(i + 1) % size];
                eqx_eclass_id_t child = id_count ? ids[bounded(data[(i + 2) % size], id_count)] : EQX_ECLASS_ID_INVALID;
                if (child != EQX_ECLASS_ID_INVALID) {
                    eqx_eclass_id_t a = eqx_egraph_add(g, (uint32_t)op_id, 1, &child);
                    if (a != EQX_ECLASS_ID_INVALID && id_count < 1024) ids[id_count++] = a;
                }
                pos += 2;
                break;
            }
            case 3: {
                uint8_t op_id = data[(i + 1) % size];
                eqx_eclass_id_t pair[2] = {EQX_ECLASS_ID_INVALID, EQX_ECLASS_ID_INVALID};
                if (id_count > 1) {
                    pair[0] = ids[bounded(data[(i + 2) % size], id_count)];
                    pair[1] = ids[bounded(data[(i + 3) % size], id_count)];
                    eqx_eclass_id_t a = eqx_egraph_add(g, (uint32_t)op_id, 2, pair);
                    if (a != EQX_ECLASS_ID_INVALID && id_count < 1024) ids[id_count++] = a;
                }
                pos += 3;
                break;
            }
            case 4: {
                if (id_count > 2) {
                    eqx_eclass_id_t tri[3];
                    tri[0] = ids[bounded(data[(i + 1) % size], id_count)];
                    tri[1] = ids[bounded(data[(i + 2) % size], id_count)];
                    tri[2] = ids[bounded(data[(i + 3) % size], id_count)];
                    eqx_eclass_id_t a = eqx_egraph_add(g, (uint32_t)data[(i + 4) % size], 3, tri);
                    if (a != EQX_ECLASS_ID_INVALID && id_count < 1024) ids[id_count++] = a;
                }
                pos += 4;
                break;
            }
            case 5: {
                if (id_count > 0) {
                    eqx_eclass_id_t a = ids[bounded(data[(i + 1) % size], id_count)];
                    eqx_eclass_id_t b = ids[bounded(data[(i + 2) % size], id_count)];
                    (void)eqx_egraph_union(g, a, b);
                }
                pos += 2;
                break;
            }
            case 6: {
                if (id_count > 0) {
                    eqx_eclass_id_t a = ids[bounded(data[(i + 1) % size], id_count)];
                    (void)eqx_egraph_union(g, a, a);
                }
                pos += 1;
                break;
            }
            case 7:
                if (id_count > 0) (void)eqx_egraph_equiv(g, ids[bounded(data[(i + 1) % size], id_count)], EQX_ECLASS_ID_INVALID);
                break;
            case 8:
            case 9:
                (void)eqx_egraph_rebuild(g);
                break;
            case 10: {
                uint8_t arity = data[(i + 1) % size] % 5;
                eqx_eclass_id_t children[4] = {0};
                for (uint8_t j = 0; j < arity; ++j) {
                    children[j] = id_count ? ids[bounded(data[(i + 2 + j) % size], id_count)] : EQX_ECLASS_ID_INVALID;
                }
                eqx_enode_t *n = eqx_enode_create((uint32_t)data[(i + 2) % size], arity, children);
                if (n) {
                    (void)eqx_hashcons_insert(hc, n, (eqx_eclass_id_t)(1 + id_count));
                    if (node_count < 1024) enodes[node_count++] = n;
                }
                pos += 1 + arity;
                break;
            }
            case 11: {
                if (node_count > 0) {
                    eqx_eclass_id_t lookup = EQX_ECLASS_ID_INVALID;
                    (void)eqx_hashcons_lookup(hc, enodes[bounded(data[(i + 1) % size], node_count)], &lookup);
                }
                pos += 1;
                break;
            }
            case 12: {
                if (node_count > 0) (void)eqx_hashcons_contains(hc, enodes[bounded(data[(i + 1) % size], node_count)]);
                pos += 1;
                break;
            }
            case 13: {
                if (node_count > 0) (void)eqx_hashcons_remove(hc, enodes[bounded(data[(i + 1) % size], node_count)]);
                pos += 1;
                break;
            }
            case 14: {
                if (node_count > 0) {
                    eqx_eclass_id_t idx = bounded(data[(i + 1) % size], node_count);
                    (void)eqx_hashcons_update(hc, enodes[idx], (eqx_eclass_id_t)(idx + 10));
                }
                pos += 1;
                break;
            }
            case 15: {
                uint8_t op_id = data[(i + 1) % size];
                eqx_pattern_t *lhs = eqx_pattern_var("x");
                eqx_pattern_t *rhs = eqx_pattern_app(op_id, NULL, 0);
                eqx_rewrite_rule_t *r = eqx_rewrite_rule_create("tmp", lhs, rhs, NULL);
                eqx_eclass_t *root = (id_count > 0) ? eqx_egraph_get_eclass(g, ids[bounded(data[(i + 2) % size], id_count)]) : NULL;
                if (root) {
                    (void)eqx_rewrite_apply(r, g, eqx_eclass_get_id(root));
                }
                eqx_rewrite_rule_destroy(r);
                pos += 2;
                break;
            }
            case 16: {
                uint8_t op_id = data[(i + 1) % size];
                eqx_pattern_t *inner = eqx_pattern_var("x");
                eqx_pattern_t *lhs = eqx_pattern_app(op_id, &inner, 1);
                eqx_pattern_t *rhs = eqx_pattern_app(op_id + 1, NULL, 0);
                eqx_rewrite_rule_t *r = eqx_rewrite_rule_create("tmp", lhs, rhs, NULL);
                eqx_rewrite_rule_t *rules[1] = {r};
                (void)eqx_rewrite_apply_rules(rules, 1, g, 8);
                eqx_rewrite_rule_destroy(r);
                pos += 1;
                break;
            }
            case 17: {
                eqx_rewrite_rule_t *r1 = eqx_rewrite_rule_create("a", eqx_pattern_var("x"), eqx_pattern_var("x"), NULL);
                eqx_rewrite_rule_t *r2 = eqx_rewrite_rule_create("b", eqx_pattern_var("x"), eqx_pattern_app(999, NULL, 0), NULL);
                eqx_rewrite_rule_t *rules[2] = {r1, r2};
                (void)eqx_rewrite_apply_rules(rules, 2, g, 4);
                eqx_rewrite_rule_destroy(r1);
                eqx_rewrite_rule_destroy(r2);
                break;
            }
            case 18:
                (void)eqx_egraph_get_stats(g, &(eqx_egraph_stats_t){0});
                break;
            case 19: {
                eqx_egraph_iter_t it = eqx_egraph_iter_begin(g);
                while (eqx_egraph_iter_has_next(it)) (void)eqx_egraph_iter_next(it);
                eqx_egraph_iter_end(it);
                break;
            }
            case 20: {
                eqx_eclass_id_t a = eqx_unionfind_make_set(uf);
                if (a != EQX_ECLASS_ID_INVALID && id_count < 1024) ids[id_count++] = a;
                break;
            }
            case 21: {
                if (id_count > 1) {
                    size_t a = bounded(data[(i + 1) % size], id_count);
                    size_t b = bounded(data[(i + 2) % size], id_count);
                    (void)eqx_unionfind_union(uf, ids[a], ids[b]);
                }
                pos += 2;
                break;
            }
            case 22: {
                if (id_count > 0) {
                    size_t a = bounded(data[(i + 1) % size], id_count);
                    (void)eqx_unionfind_find(uf, ids[a]);
                    (void)eqx_unionfind_find(uf, ids[a]);
                }
                pos += 1;
                break;
            }
            case 23: {
                if (id_count > 1) {
                    size_t a = bounded(data[(i + 1) % size], id_count);
                    size_t b = bounded(data[(i + 2) % size], id_count);
                    (void)eqx_unionfind_equiv(uf, ids[a], ids[b]);
                }
                pos += 2;
                break;
            }
            case 24: {
                if (id_count > 0) {
                    size_t a = bounded(data[(i + 1) % size], id_count);
                    (void)eqx_egraph_get_eclass(g, ids[a]);
                }
                pos += 1;
                break;
            }
            case 25:
                if (id_count > 0) (void)eqx_egraph_find(g, ids[bounded(data[(i + 1) % size], id_count)]);
                pos += 1;
                break;
            case 26: {
                uint8_t op_id = data[(i + 1) % size];
                eqx_eclass_id_t a = eqx_egraph_add(g, op_id, 0, NULL);
                eqx_eclass_id_t b = eqx_egraph_add(g, op_id, 0, NULL);
                if (a != EQX_ECLASS_ID_INVALID && b != EQX_ECLASS_ID_INVALID) {
                    (void)eqx_egraph_union(g, a, b);
                    (void)eqx_egraph_equiv(g, a, b);
                    if (id_count < 1024) {
                        ids[id_count++] = a;
                        if (id_count < 1024) ids[id_count++] = b;
                    }
                }
                pos += 1;
                break;
            }
            case 27:
                if (id_count > 0) {
                    eqx_eclass_id_t child = ids[bounded(data[(i + 2) % size], id_count)];
                    eqx_eclass_id_t* child_arr = &child;
                    eqx_egraph_add(g, (uint32_t)data[(i + 1) % size], 1, child_arr);
                }
                pos += 2;
                break;
            case 28:
                if (id_count > 0) {
                    eqx_subst_t *subst = eqx_subst_create();
                    eqx_pattern_t *p = eqx_pattern_var("x");
                    if (subst) {
                        (void)eqx_pattern_match(p, g, ids[bounded(data[(i + 1) % size], id_count)], subst);
                        eqx_subst_destroy(subst);
                    }
                    eqx_pattern_destroy(p);
                }
                pos += 1;
                break;
            case 29:
                (void)eqx_egraph_num_eclasses(g);
                break;
            case 30:
                (void)eqx_egraph_get_stats(g, &(eqx_egraph_stats_t){0});
                break;
            default:
                break;
        }

        ++pos;
        if (pos >= size) break;
    }

    eqx_hashcons_destroy(hc);
    eqx_unionfind_destroy(uf);
    for (size_t i = 0; i < node_count; ++i) {
        eqx_enode_destroy(enodes[i]);
    }
    eqx_egraph_destroy(g);
    return 0;
}
