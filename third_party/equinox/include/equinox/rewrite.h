#ifndef EQUINOX_REWRITE_H
#define EQUINOX_REWRITE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>
#include "equinox/enode.h"
#include "equinox/egraph.h"

typedef enum {
    EQX_PATTERN_VAR = 0,
    EQX_PATTERN_APP = 1
} eqx_pattern_type_t;

typedef struct eqx_pattern {
    eqx_pattern_type_t type;
    union {
        struct {
            const char *name;
        } var;
        struct {
            uint32_t op;
            size_t arity;
            struct eqx_pattern** children;
        } app;
    } data;
} eqx_pattern_t;

typedef struct eqx_subst_entry {
    const char* var_name;
    eqx_eclass_id_t eclass_id;
    struct eqx_subst_entry* next;
} eqx_subst_entry_t;

typedef struct eqx_subst {
    eqx_subst_entry_t* head;
} eqx_subst_t;

typedef bool (*eqx_rewrite_condition_fn)(const eqx_subst_t*, void*);

typedef struct eqx_rewrite_rule {
    char* name;
    eqx_pattern_t* lhs;
    eqx_pattern_t* rhs;
    eqx_rewrite_condition_fn condition;
    void* condition_data;
} eqx_rewrite_rule_t;

/* Pattern API */
eqx_pattern_t* eqx_pattern_var(const char *name);
eqx_pattern_t* eqx_pattern_app(uint32_t op, eqx_pattern_t **children, size_t arity);
void eqx_pattern_destroy(eqx_pattern_t* pattern);
bool eqx_pattern_is_var(const eqx_pattern_t* pattern);
const char* eqx_pattern_get_var_name(const eqx_pattern_t* pattern);
uint32_t eqx_pattern_get_operator(const eqx_pattern_t* pattern);
size_t eqx_pattern_get_arity(const eqx_pattern_t* pattern);
eqx_pattern_t* eqx_pattern_get_child(const eqx_pattern_t* pattern, size_t index);

/* Substitution API */
eqx_subst_t* eqx_subst_create(void);
void eqx_subst_destroy(eqx_subst_t* subst);
bool eqx_subst_add(eqx_subst_t* subst, const char* name, eqx_eclass_id_t id);
bool eqx_subst_lookup(const eqx_subst_t* subst, const char* name, eqx_eclass_id_t* out_id);

/* Rewrite operations */
bool eqx_pattern_match(eqx_pattern_t* pattern, const eqx_egraph_t* egraph, eqx_eclass_id_t eclass_id, eqx_subst_t* subst);
eqx_eclass_id_t eqx_pattern_instantiate(const eqx_pattern_t* pattern,
                                      eqx_egraph_t* egraph,
                                      const eqx_subst_t* subst);
eqx_rewrite_rule_t* eqx_rewrite_rule_create(const char* name, eqx_pattern_t* lhs, eqx_pattern_t* rhs,
                                          eqx_rewrite_condition_fn condition);
void eqx_rewrite_rule_destroy(eqx_rewrite_rule_t* rule);

size_t eqx_rewrite_apply(eqx_rewrite_rule_t* rule, eqx_egraph_t* egraph, eqx_eclass_id_t eclass_id);
size_t eqx_rewrite_apply_all(eqx_rewrite_rule_t* rule, eqx_egraph_t* egraph);
size_t eqx_rewrite_apply_rules(eqx_rewrite_rule_t** rules, size_t num_rules, eqx_egraph_t* egraph, size_t max_iterations);

#ifdef __cplusplus
}
#endif

#endif /* EQUINOX_REWRITE_H */
