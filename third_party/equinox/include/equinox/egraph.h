#ifndef EQUINOX_EGRAPH_H
#define EQUINOX_EGRAPH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "eclass.h"
#include "enode.h"
#include "unionfind.h"

typedef uint32_t eqx_operator_t;
typedef eclass_id_t eqx_eclass_id_t;

typedef struct eqx_egraph eqx_egraph_t;
typedef struct eqx_egraph_stats {
    size_t num_enodes;
    size_t num_eclasses;
    size_t num_unions;
    size_t hashcons_size;
} eqx_egraph_stats_t;

typedef struct eqx_egraph_config {
    size_t initial_capacity;
    bool enable_explanations;
    size_t max_iterations;
    size_t node_limit;
} eqx_egraph_config_t;

typedef struct eqx_egraph_iter* eqx_egraph_iter_t;

eqx_egraph_config_t eqx_egraph_config_default(void);
eqx_egraph_t* eqx_egraph_create(const eqx_egraph_config_t* config);
void eqx_egraph_destroy(eqx_egraph_t* egraph);

eqx_eclass_id_t eqx_egraph_add(eqx_egraph_t* egraph, eqx_operator_t op,
                                size_t arity, const eqx_eclass_id_t* children);
bool eqx_egraph_union(eqx_egraph_t* egraph, eqx_eclass_id_t id1, eqx_eclass_id_t id2);
eqx_eclass_id_t eqx_egraph_find(eqx_egraph_t* egraph, eqx_eclass_id_t id);
bool eqx_egraph_equiv(eqx_egraph_t* egraph, eqx_eclass_id_t id1, eqx_eclass_id_t id2);
bool eqx_egraph_rebuild(eqx_egraph_t* egraph);

size_t eqx_egraph_num_eclasses(const eqx_egraph_t* egraph);
eqx_eclass_t* eqx_egraph_get_eclass(eqx_egraph_t* egraph, eqx_eclass_id_t id);
void eqx_egraph_get_stats(const eqx_egraph_t* egraph, eqx_egraph_stats_t* out_stats);

/* Iterator API */
eqx_egraph_iter_t eqx_egraph_iter_begin(eqx_egraph_t* egraph);
bool eqx_egraph_iter_has_next(const eqx_egraph_iter_t iter);
eqx_eclass_id_t eqx_egraph_iter_next(eqx_egraph_iter_t iter);
void eqx_egraph_iter_end(eqx_egraph_iter_t iter);

#ifdef __cplusplus
}
#endif

#endif /* EQUINOX_EGRAPH_H */
