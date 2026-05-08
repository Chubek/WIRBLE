#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "equinox/hashcons.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    eqx_hashcons_t *hc = eqx_hashcons_create(4, 0.75);
    if (!hc) return 0;

    eqx_enode_t *nodes[1024] = {0};
    size_t count = 0;
    for (size_t i = 0; i + 1 < size && count < 1024; ++i) {
        uint8_t op = data[i] % 4;
        if (op == 0) {
            eqx_eclass_id_t children[4] = {0};
            uint8_t arity = data[(i + 1) % size] % 5;
            for (uint8_t j = 0; j < arity; ++j) {
                children[j] = (eqx_eclass_id_t) data[(i + 2 + j) % size];
            }

            eqx_enode_t *node = eqx_enode_create((uint32_t)data[(i + 2) % size], arity, children);
            if (node) {
                eqx_hashcons_insert(hc, node, (eqx_eclass_id_t)(i & 0xFFFF));
                if (count < 1024) nodes[count++] = node;
            }
            i += 2 + arity;
        } else if (op == 1 && count > 0) {
            size_t idx = data[(i + 1) % size] % (uint8_t)count;
            (void)eqx_hashcons_contains(hc, nodes[idx]);
            eqx_eclass_id_t id;
            (void)eqx_hashcons_lookup(hc, nodes[idx], &id);
            i += 1;
        } else if (op == 2 && count > 0) {
            size_t idx = data[(i + 1) % size] % (uint8_t)count;
            (void)eqx_hashcons_remove(hc, nodes[idx]);
            i += 1;
        } else if (op == 3 && count > 0) {
            size_t idx = data[(i + 1) % size] % (uint8_t)count;
            eqx_hashcons_update(hc, nodes[idx], (eqx_eclass_id_t)(1 + i));
            i += 1;
        }
    }

    eqx_hashcons_destroy(hc);
    for (size_t i = 0; i < count; ++i) {
        eqx_enode_destroy(nodes[i]);
    }
    return 0;
}
