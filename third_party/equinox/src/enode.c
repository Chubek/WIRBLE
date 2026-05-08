#include <equinox/egraph.h>
#include <equinox/eclass.h>
#include <equinox/enode.h>
#include <equinox/libutils.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ============================================================================
 * E-node Creation and Destruction
 * ========================================================================== */

eqx_enode_t* eqx_enode_create(eqx_symbol_t op, size_t arity, const eqx_eclass_id_t* children) {
    assert(arity == 0 || children != NULL);
    
    eqx_enode_t* node = (eqx_enode_t*)malloc(sizeof(eqx_enode_t));
    if (!node) return NULL;
    
    node->op = op;
    node->arity = arity;
    node->eclass = EQX_ECLASS_ID_INVALID;
    node->next_in_class = NULL;
    node->cost = 1.0;
    
    if (arity > 0) {
        node->children = (eqx_eclass_id_t*)malloc(arity * sizeof(eqx_eclass_id_t));
        if (!node->children) {
            free(node);
            return NULL;
        }
        memcpy(node->children, children, arity * sizeof(eqx_eclass_id_t));
    } else {
        node->children = NULL;
    }
    
    return node;
}

void eqx_enode_destroy(eqx_enode_t* node) {
    if (!node) return;
    
    if (node->children) {
        free(node->children);
    }
    free(node);
}

/* ============================================================================
 * E-node Accessors
 * ========================================================================== */

eqx_symbol_t eqx_enode_get_op(const eqx_enode_t* node) {
    assert(node != NULL);
    return node->op;
}

size_t eqx_enode_get_arity(const eqx_enode_t* node) {
    assert(node != NULL);
    return node->arity;
}

eqx_eclass_id_t eqx_enode_get_child(const eqx_enode_t* node, size_t index) {
    assert(node != NULL);
    assert(index < node->arity);
    return node->children[index];
}

const eqx_eclass_id_t* eqx_enode_get_children(const eqx_enode_t* node) {
    assert(node != NULL);
    return node->children;
}

eqx_eclass_id_t eqx_enode_get_eclass(const eqx_enode_t* node) {
    assert(node != NULL);
    return node->eclass;
}

void eqx_enode_set_eclass(eqx_enode_t* node, eqx_eclass_id_t eclass) {
    assert(node != NULL);
    node->eclass = eclass;
}

double eqx_enode_get_cost(const eqx_enode_t* node) {
    assert(node != NULL);
    return node->cost;
}

void eqx_enode_set_cost(eqx_enode_t* node, double cost) {
    assert(node != NULL);
    node->cost = cost;
}

/* ============================================================================
 * E-node Comparison and Hashing
 * ========================================================================== */

bool eqx_enode_equal(const eqx_enode_t* a, const eqx_enode_t* b) {
    if (a == b) return true;
    if (!a || !b) return false;
    
    if (a->op != b->op) return false;
    if (a->arity != b->arity) return false;
    
    for (size_t i = 0; i < a->arity; i++) {
        if (a->children[i] != b->children[i]) {
            return false;
        }
    }
    
    return true;
}

uint32_t eqx_enode_hash(const eqx_enode_t* node) {
    assert(node != NULL);
    
    // Use FNV-1a hash from libutils
    uint32_t hash = 2166136261u;
    
    // Hash the operator
    hash ^= (uint32_t)node->op;
    hash *= 16777619u;
    
    // Hash the arity
    hash ^= (uint32_t)node->arity;
    hash *= 16777619u;
    
    // Hash each child e-class ID
    for (size_t i = 0; i < node->arity; i++) {
        hash ^= (uint32_t)node->children[i];
        hash *= 16777619u;
    }
    
    return hash;
}

/* ============================================================================
 * E-node Canonicalization
 * ========================================================================== */

void eqx_enode_canonicalize(eqx_enode_t* node, struct unionfind* uf) {
    assert(node != NULL);
    assert(uf != NULL);
    
    for (size_t i = 0; i < node->arity; i++) {
        node->children[i] = eqx_unionfind_find(uf, node->children[i]);
    }
}

/* ============================================================================
 * E-node Utilities
 * ========================================================================== */

eqx_enode_t* eqx_enode_clone(const eqx_enode_t* node) {
    if (!node) return NULL;
    
    return eqx_enode_create(node->op, node->arity, node->children);
}

void eqx_enode_print(const eqx_enode_t* node, FILE* out) {
    if (!node) {
        fprintf(out, "<null>");
        return;
    }
    
    fprintf(out, "%lu(", (unsigned long)node->op);
    
    for (size_t i = 0; i < node->arity; i++) {
        if (i > 0) fprintf(out, ", ");
        fprintf(out, "e%lu", (unsigned long)node->children[i]);
    }
    
    fprintf(out, ")");
    
    if (node->eclass != EQX_ECLASS_ID_INVALID) {
        fprintf(out, " [class=e%lu]", (unsigned long)node->eclass);
    }
    
    if (node->cost != 1.0) {
        fprintf(out, " [cost=%.2f]", node->cost);
    }
}
