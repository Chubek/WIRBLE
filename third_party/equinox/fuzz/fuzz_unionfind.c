#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "equinox/unionfind.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    eqx_unionfind_t *uf = eqx_unionfind_create(16);
    if (!uf) return 0;

    eqx_eclass_id_t ids[10240];
    size_t count = 0;

    for (size_t i = 0; i < size && count < 10240; ++i) {
        uint8_t op = data[i] % 5;
        if (op == 0) {
            eqx_eclass_id_t id = eqx_unionfind_make_set(uf);
            if (id != EQX_ECLASS_ID_INVALID) ids[count++] = id;
        } else if (op == 1 && count > 1) {
            size_t a = data[(i + 1) % size] % count;
            size_t b = data[(i + 2) % size] % count;
            (void)eqx_unionfind_union(uf, ids[a], ids[b]);
            i += 2;
        } else if (op == 2 && count > 0) {
            size_t a = data[(i + 1) % size] % count;
            (void)eqx_unionfind_find(uf, ids[a]);
            (void)eqx_unionfind_find(uf, ids[a]);
            i += 1;
        } else if (op == 3 && count > 1) {
            size_t a = data[(i + 1) % size] % count;
            size_t b = data[(i + 2) % size] % count;
            (void)eqx_unionfind_equiv(uf, ids[a], ids[b]);
            i += 2;
        } else if (op == 4 && count > 0) {
            size_t a = data[(i + 1) % size] % count;
            size_t bad = count + (data[(i + 2) % size]);
            (void)eqx_unionfind_union(uf, ids[a], (eqx_eclass_id_t)bad);
            i += 2;
        }
    }

    eqx_unionfind_destroy(uf);
    return 0;
}
