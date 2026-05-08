#ifndef EQUINOX_HASHCONS_H
#define EQUINOX_HASHCONS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>
#include "equinox/enode.h"

typedef struct eqx_hashcons eqx_hashcons_t;
typedef struct eqx_hashcons_iter *eqx_hashcons_iter_t;

/* Create/destroy */
eqx_hashcons_t* eqx_hashcons_create(size_t initial_capacity, double load_factor);
void eqx_hashcons_destroy(eqx_hashcons_t* hc);

/* Insert/lookup/remove */
bool eqx_hashcons_insert(eqx_hashcons_t* hc, const eqx_enode_t* node, eqx_eclass_id_t eclass_id);
bool eqx_hashcons_lookup(const eqx_hashcons_t* hc, const eqx_enode_t* node, eqx_eclass_id_t* out_id);
bool eqx_hashcons_contains(const eqx_hashcons_t* hc, const eqx_enode_t* node);
bool eqx_hashcons_remove(eqx_hashcons_t* hc, const eqx_enode_t* node);
size_t eqx_hashcons_size(const eqx_hashcons_t* hc);

/* Optional helpers used by egraph internals */
void eqx_hashcons_update(eqx_hashcons_t* hc, const eqx_enode_t* node, eqx_eclass_id_t new_id);

/* Iterator */
eqx_hashcons_iter_t eqx_hashcons_iter_begin(const eqx_hashcons_t* hc);
bool eqx_hashcons_iter_has_next(eqx_hashcons_iter_t iter);
void eqx_hashcons_iter_next(eqx_hashcons_iter_t iter, eqx_enode_t** node, eqx_eclass_id_t* out_id);
void eqx_hashcons_iter_end(eqx_hashcons_iter_t iter);

#ifdef __cplusplus
}
#endif

#endif /* EQUINOX_HASHCONS_H */
