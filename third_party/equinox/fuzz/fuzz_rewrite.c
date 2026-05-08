#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "equinox/rewrite.h"

static eqx_rewrite_rule_t *build_rule_cycle(void) {
    eqx_pattern_t *a = eqx_pattern_app(1, (eqx_pattern_t*[]){eqx_pattern_var("x")}, 1);
    eqx_pattern_t *b = eqx_pattern_app(2, (eqx_pattern_t*[]){eqx_pattern_var("x")}, 1);
    return eqx_rewrite_rule_create("cycle", a, b, NULL);
}

static eqx_rewrite_rule_t *build_rule_reset(void) {
    eqx_pattern_t *a = eqx_pattern_app(2, (eqx_pattern_t*[]){eqx_pattern_var("x")}, 1);
    eqx_pattern_t *b = eqx_pattern_app(1, (eqx_pattern_t*[]){eqx_pattern_var("x")}, 1);
    return eqx_rewrite_rule_create("reset", a, b, NULL);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    eqx_egraph_t *g = eqx_egraph_create(NULL);
    if (!g) return 0;

    eqx_eclass_id_t ids[1024];
    size_t count = 0;
    for (size_t i = 0; i < size && count < 1024; ++i) {
        uint8_t op = data[i] % 3;
        if (op == 0) {
            uint8_t arity = data[(i + 1) % size] % 5;
            eqx_eclass_id_t children[4] = {0};
            for (uint8_t j = 0; j < arity && count > 0; ++j) {
                children[j] = ids[data[(i + 2 + j) % size] % count];
            }
            eqx_eclass_id_t id = eqx_egraph_add(g, (uint32_t)data[(i + 1) % size], arity, children);
            if (id != EQX_ECLASS_ID_INVALID && count < 1024) ids[count++] = id;
            i += 1 + arity;
        } else if (op == 1) {
            eqx_rewrite_rule_t *r1 = build_rule_cycle();
            eqx_rewrite_rule_t *r2 = build_rule_reset();
            eqx_rewrite_rule_t *rules[2] = {r1, r2};
            (void)eqx_rewrite_apply_rules(rules, 2, g, 8);
            eqx_rewrite_rule_destroy(r1);
            eqx_rewrite_rule_destroy(r2);
        } else {
            eqx_rewrite_rule_t *noop = eqx_rewrite_rule_create("noop",
                eqx_pattern_var("x"), eqx_pattern_var("x"), NULL);
            if (count > 0) {
                (void)eqx_rewrite_apply(noop, g, ids[data[(i + 1) % size] % count]);
            }
            eqx_rewrite_rule_destroy(noop);
        }
    }

    (void)eqx_egraph_rebuild(g);
    eqx_egraph_destroy(g);
    return 0;
}
