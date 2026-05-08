#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "equinox/egraph.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    eqx_egraph_t *g = eqx_egraph_create(NULL);
    if (!g) return 0;

    eqx_eclass_id_t ids[1024] = {0};
    size_t count = 0;

    for (size_t i = 0; i < size && count < 1024; ++i) {
        if (data[i] == 0) {
            eqx_eclass_id_t id = eqx_egraph_add(g, (uint32_t)(data[(i + 1) % size]), 0, NULL);
            if (id != EQX_ECLASS_ID_INVALID && count < 1024) ids[count++] = id;
            continue;
        }

        if (data[i] == 1 && count >= 2) {
            size_t a = data[(i + 1) % size] % count;
            size_t b = data[(i + 2) % size] % count;
            (void)eqx_egraph_union(g, ids[a], ids[b]);
            if ((data[i] & 1) != 0) {
                (void)eqx_egraph_rebuild(g);
            }
        } else if (data[i] == 2) {
            (void)eqx_egraph_rebuild(g);
        } else if (data[i] == 3 && count > 0) {
            size_t a = data[(i + 1) % size] % count;
            size_t b = data[(i + 2) % size] % count;
            (void)eqx_egraph_union(g, ids[a], ids[b]);
            (void)eqx_egraph_union(g, ids[a], ids[a]);
            (void)eqx_egraph_union(g, ids[b], ids[b]);
        } else {
            if (count > 1) {
                size_t x = data[(i + 1) % size] % count;
                size_t y = (x + 1) % count;
                (void)eqx_egraph_union(g, ids[x], ids[y]);
            }
        }
    }

    eqx_egraph_destroy(g);
    return 0;
}
