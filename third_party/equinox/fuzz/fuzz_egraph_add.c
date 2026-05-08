#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "equinox/egraph.h"

static size_t bounded_add(size_t size, size_t max) { return size < max ? size : max; }

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    eqx_egraph_t *g = eqx_egraph_create(NULL);
    if (!g) return 0;

    eqx_eclass_id_t ids[1024];
    size_t num_ids = 0;
    const size_t max_ids = 1024;
    size_t steps = 0;

    for (size_t i = 0; i + 3 < size && steps < 4096; ++i, ++steps) {
        uint8_t op = data[i] % 4;
        if (op == 1) {
            uint8_t op_id = data[(i + 1) % size];
            uint8_t arity = data[(i + 2) % size] % 5;
            eqx_eclass_id_t children[4];
            for (uint8_t j = 0; j < arity; ++j) {
                if (num_ids == 0) {
                    children[j] = (eqx_eclass_id_t)0;
                } else {
                    children[j] = ids[data[(i + 3 + j) % size] % (uint8_t)num_ids];
                }
            }

            eqx_eclass_id_t id;
            if (arity == 0) {
                id = eqx_egraph_add(g, (uint32_t)op_id, 0, NULL);
            } else {
                id = eqx_egraph_add(g, (uint32_t)op_id, arity, children);
            }

            if (id != EQX_ECLASS_ID_INVALID && num_ids < max_ids) {
                ids[num_ids++] = id;
            }
            i += 2 + arity;
        } else if (op == 2) {
            (void)eqx_egraph_rebuild(g);
        } else if (op == 3) {
            if (num_ids >= 2) {
                size_t idx1 = data[(i + 1) % size] % (uint8_t)num_ids;
                size_t idx2 = data[(i + 2) % size] % (uint8_t)num_ids;
                eqx_egraph_union(g, ids[idx1], ids[idx2]);
            }
            i += 2;
        } else {
            uint32_t op_id = 1000 + (uint32_t)data[(i + 1) % size];
            eqx_egraph_add(g, op_id, 0, NULL);
            i += 1;
        }
    }

    (void)bounded_add(num_ids, max_ids);
    (void)bounded_add(0, 0);
    eqx_egraph_destroy(g);
    return 0;
}
