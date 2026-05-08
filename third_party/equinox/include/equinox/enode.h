#ifndef EQUINOX_ENODE_H
#define EQUINOX_ENODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* Forward declarations */
typedef struct enode enode_t;
typedef struct eclass eclass_t;
typedef uint32_t eclass_id_t;
struct unionfind;

/* ENode represents a term node in the e-graph */
struct enode {
    /* Operator/symbol identifier */
    uint32_t op;
    
    /* Number of children */
    size_t arity;
    
    /* Array of child eclass IDs */
    eclass_id_t* children;
    
    /* Parent eclass this enode belongs to */
    eclass_id_t eclass;
    
    /* Hash value for hashconsing */
    uint64_t hash;
    
    /* User data pointer */
    void* user_data;

    /* Legacy fields used by eqx_* API in source/tests */
    struct enode* next_in_class;
    double cost;
};

/* ENode creation and destruction */
enode_t* enode_create(uint32_t op, size_t arity, const eclass_id_t* children);
void enode_destroy(enode_t* node);

/* ENode operations */
uint64_t enode_hash(const enode_t* node);
bool enode_equal(const enode_t* a, const enode_t* b);
enode_t* enode_clone(const enode_t* node);

/* Accessors */
uint32_t enode_get_op(const enode_t* node);
size_t enode_get_arity(const enode_t* node);
eclass_id_t enode_get_child(const enode_t* node, size_t index);
eclass_id_t enode_get_eclass(const enode_t* node);

/* Setters */
void enode_set_eclass(enode_t* node, eclass_id_t eclass);
void enode_set_user_data(enode_t* node, void* data);
void* enode_get_user_data(const enode_t* node);

/* =========================================================================
 * Legacy API compatibility (eqx_ prefix)
 * ========================================================================= */
typedef eclass_id_t eqx_eclass_id_t;
typedef uint32_t eqx_symbol_t;
typedef enode_t eqx_enode_t;

#ifndef EQX_ECLASS_ID_INVALID
#define EQX_ECLASS_ID_INVALID ((eqx_eclass_id_t)-1)
#endif

eqx_enode_t* eqx_enode_create(eqx_symbol_t op, size_t arity, const eqx_eclass_id_t* children);
void eqx_enode_destroy(eqx_enode_t* node);
eqx_symbol_t eqx_enode_get_op(const eqx_enode_t* node);
size_t eqx_enode_get_arity(const eqx_enode_t* node);
eqx_eclass_id_t eqx_enode_get_child(const eqx_enode_t* node, size_t index);
const eqx_eclass_id_t* eqx_enode_get_children(const eqx_enode_t* node);
eqx_eclass_id_t eqx_enode_get_eclass(const eqx_enode_t* node);
void eqx_enode_set_eclass(eqx_enode_t* node, eqx_eclass_id_t eclass);
double eqx_enode_get_cost(const eqx_enode_t* node);
void eqx_enode_set_cost(eqx_enode_t* node, double cost);
bool eqx_enode_equal(const eqx_enode_t* a, const eqx_enode_t* b);
uint32_t eqx_enode_hash(const eqx_enode_t* node);
void eqx_enode_canonicalize(eqx_enode_t* node, struct unionfind* uf);
eqx_enode_t* eqx_enode_clone(const eqx_enode_t* node);
void eqx_enode_print(const eqx_enode_t* node, FILE* out);

/* Legacy convenience aliases */
#define eqx_enode_get_operator eqx_enode_get_op

/* Canonicalization - replace children with canonical eclass IDs */
void enode_canonicalize(enode_t* node, eclass_id_t* (*find_fn)(eclass_id_t));

/* String representation for debugging */
char* enode_to_string(const enode_t* node);

#ifdef __cplusplus
}
#endif

#endif /* EQUINOX_ENODE_H */
