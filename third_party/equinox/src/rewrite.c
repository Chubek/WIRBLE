#include "equinox/rewrite.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

eqx_pattern_t* eqx_pattern_var(const char *name) {
    assert(name);
    eqx_pattern_t* p = malloc(sizeof(eqx_pattern_t));
    if (!p) return NULL;
    p->type = EQX_PATTERN_VAR;
    p->data.var.name = name;
    return p;
}

eqx_pattern_t* eqx_pattern_app(uint32_t op, eqx_pattern_t **children, size_t arity) {
    assert(children || arity == 0);
    eqx_pattern_t* p = malloc(sizeof(eqx_pattern_t));
    if (!p) return NULL;
    p->type = EQX_PATTERN_APP;
    p->data.app.op = op;
    p->data.app.arity = arity;
    if (arity > 0) {
        p->data.app.children = malloc(arity * sizeof(eqx_pattern_t*));
        if (!p->data.app.children) {
            free(p);
            return NULL;
        }
        memcpy(p->data.app.children, children, arity * sizeof(eqx_pattern_t*));
    } else {
        p->data.app.children = NULL;
    }
    return p;
}

void eqx_pattern_destroy(eqx_pattern_t* pattern) {
    if (!pattern) return;
    if (pattern->type == EQX_PATTERN_APP) {
        for (size_t i = 0; i < pattern->data.app.arity; i++) {
            eqx_pattern_destroy(pattern->data.app.children[i]);
        }
        free(pattern->data.app.children);
    }
    free(pattern);
}

bool eqx_pattern_is_var(const eqx_pattern_t* pattern) {
    return pattern && pattern->type == EQX_PATTERN_VAR;
}

const char* eqx_pattern_get_var_name(const eqx_pattern_t* pattern) {
    assert(pattern && pattern->type == EQX_PATTERN_VAR);
    return pattern->data.var.name;
}

uint32_t eqx_pattern_get_operator(const eqx_pattern_t* pattern) {
    assert(pattern && pattern->type == EQX_PATTERN_APP);
    return pattern->data.app.op;
}

size_t eqx_pattern_get_arity(const eqx_pattern_t* pattern) {
    assert(pattern && pattern->type == EQX_PATTERN_APP);
    return pattern->data.app.arity;
}

eqx_pattern_t* eqx_pattern_get_child(const eqx_pattern_t* pattern, size_t index) {
    assert(pattern && pattern->type == EQX_PATTERN_APP);
    assert(index < pattern->data.app.arity);
    return pattern->data.app.children[index];
}

/* Substitution API */
eqx_subst_t* eqx_subst_create(void) {
    eqx_subst_t* s = malloc(sizeof(eqx_subst_t));
    if (!s) return NULL;
    s->head = NULL;
    return s;
}

void eqx_subst_destroy(eqx_subst_t* subst) {
    if (!subst) return;
    eqx_subst_entry_t* it = subst->head;
    while (it) {
        eqx_subst_entry_t* next = it->next;
        free(it);
        it = next;
    }
    free(subst);
}

bool eqx_subst_add(eqx_subst_t* subst, const char* name, eqx_eclass_id_t id) {
    assert(subst);
    assert(name);

    for (eqx_subst_entry_t* it = subst->head; it; it = it->next) {
        if (strcmp(it->var_name, name) == 0) {
            return it->eclass_id == id;
        }
    }

    eqx_subst_entry_t* entry = malloc(sizeof(eqx_subst_entry_t));
    if (!entry) return false;
    entry->var_name = name;
    entry->eclass_id = id;
    entry->next = subst->head;
    subst->head = entry;
    return true;
}

bool eqx_subst_lookup(const eqx_subst_t* subst, const char* name, eqx_eclass_id_t* out_id) {
    for (const eqx_subst_entry_t* it = subst ? subst->head : NULL; it; it = it->next) {
        if (strcmp(it->var_name, name) == 0) {
            if (out_id) *out_id = it->eclass_id;
            return true;
        }
    }
    return false;
}

static bool match_pattern_internal(const eqx_egraph_t* egraph,
                                 const eqx_pattern_t* pattern,
                                 eqx_eclass_id_t eclass_id,
                                 eqx_subst_t* subst) {
    if (pattern->type == EQX_PATTERN_VAR) {
        return eqx_subst_add(subst, pattern->data.var.name, eclass_id);
    }

    eqx_eclass_t* eclass = eqx_egraph_get_eclass((eqx_egraph_t*)egraph, eclass_id);
    if (!eclass) return false;

    for (eqx_enode_t* node = eclass->nodes; node != NULL; node = node->next_in_class) {
        if (node->op != pattern->data.app.op || node->arity != pattern->data.app.arity) {
            continue;
        }

        bool ok = true;
        for (size_t i = 0; i < pattern->data.app.arity; i++) {
            if (!match_pattern_internal(egraph, pattern->data.app.children[i], node->children[i], subst)) {
                ok = false;
                break;
            }
        }

        if (ok) return true;
    }

    return false;
}

bool eqx_pattern_match(eqx_pattern_t* pattern, const eqx_egraph_t* egraph, eqx_eclass_id_t eclass_id, eqx_subst_t* subst) {
    assert(pattern);
    assert(egraph);
    assert(subst);
    return match_pattern_internal(egraph, pattern, eclass_id, subst);
}

eqx_eclass_id_t eqx_pattern_instantiate(const eqx_pattern_t* pattern,
                                      eqx_egraph_t* egraph,
                                      const eqx_subst_t* subst) {
    assert(pattern);
    assert(egraph);
    assert(subst);

    if (pattern->type == EQX_PATTERN_VAR) {
        eqx_eclass_id_t id;
        if (!eqx_subst_lookup(subst, pattern->data.var.name, &id)) {
            return EQX_ECLASS_ID_INVALID;
        }
        return id;
    }

    eqx_eclass_id_t* child_ids = malloc(pattern->data.app.arity * sizeof(eqx_eclass_id_t));
    if (!child_ids && pattern->data.app.arity > 0) return EQX_ECLASS_ID_INVALID;

    for (size_t i = 0; i < pattern->data.app.arity; i++) {
        child_ids[i] = eqx_pattern_instantiate(pattern->data.app.children[i], egraph, subst);
        if (child_ids[i] == EQX_ECLASS_ID_INVALID) {
            free(child_ids);
            return EQX_ECLASS_ID_INVALID;
        }
    }

    eqx_eclass_id_t result = eqx_egraph_add(egraph, pattern->data.app.op, pattern->data.app.arity, child_ids);
    free(child_ids);
    return result;
}

eqx_rewrite_rule_t* eqx_rewrite_rule_create(const char* name, eqx_pattern_t* lhs, eqx_pattern_t* rhs,
                                          eqx_rewrite_condition_fn condition) {
    assert(name);
    assert(lhs);
    assert(rhs);
    eqx_rewrite_rule_t* rule = malloc(sizeof(eqx_rewrite_rule_t));
    if (!rule) return NULL;
    rule->name = strdup(name);
    if (!rule->name) {
        free(rule);
        return NULL;
    }
    rule->lhs = lhs;
    rule->rhs = rhs;
    rule->condition = condition;
    rule->condition_data = NULL;
    return rule;
}

void eqx_rewrite_rule_destroy(eqx_rewrite_rule_t* rule) {
    if (!rule) return;
    eqx_pattern_destroy(rule->lhs);
    eqx_pattern_destroy(rule->rhs);
    free(rule->name);
    free(rule);
}

static bool rule_condition_satisfied(const eqx_rewrite_rule_t* rule,
                                    const eqx_egraph_t* egraph,
                                    const eqx_subst_t* subst) {
    (void)egraph;
    return !rule || !rule->condition || rule->condition(subst, rule->condition_data);
}

size_t eqx_rewrite_apply(eqx_rewrite_rule_t* rule, eqx_egraph_t* egraph, eqx_eclass_id_t eclass_id) {
    assert(rule);
    assert(egraph);

    size_t rewrites = 0;
    eqx_subst_t* subst = eqx_subst_create();
    if (!subst) return 0;

    if (match_pattern_internal(egraph, rule->lhs, eclass_id, subst) &&
        rule_condition_satisfied(rule, egraph, subst)) {
        eqx_eclass_id_t rhs_id = eqx_pattern_instantiate(rule->rhs, egraph, subst);
        if (rhs_id != EQX_ECLASS_ID_INVALID) {
            eqx_egraph_union(egraph, eclass_id, rhs_id);
            rewrites = 1;
        }
    }

    eqx_subst_destroy(subst);
    return rewrites;
}

size_t eqx_rewrite_apply_all(eqx_rewrite_rule_t* rule, eqx_egraph_t* egraph) {
    assert(rule);
    assert(egraph);

    size_t total = 0;
    eqx_egraph_iter_t it = eqx_egraph_iter_begin(egraph);
    while (eqx_egraph_iter_has_next(it)) {
        eqx_eclass_id_t id = eqx_egraph_iter_next(it);
        total += eqx_rewrite_apply(rule, egraph, id);
    }
    eqx_egraph_iter_end(it);
    return total;
}

size_t eqx_rewrite_apply_rules(eqx_rewrite_rule_t** rules, size_t num_rules, eqx_egraph_t* egraph, size_t max_iterations) {
    assert(egraph);
    if (!rules && num_rules > 0) return 0;

    size_t total = 0;
    for (size_t iter = 0; iter < max_iterations; iter++) {
        size_t count = 0;
        for (size_t i = 0; i < num_rules; i++) {
            count += eqx_rewrite_apply_all(rules[i], egraph);
        }
        total += count;
        if (count == 0) break;
        if (!eqx_egraph_rebuild(egraph)) break;
    }
    return total;
}
