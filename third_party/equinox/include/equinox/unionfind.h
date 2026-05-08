#ifndef EQUINOX_UNIONFIND_H
#define EQUINOX_UNIONFIND_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Forward declaration */
typedef struct unionfind unionfind_t;

/* Union-Find element ID type */
typedef uint32_t uf_id_t;

/* Invalid ID constant */
#define UF_ID_INVALID ((uf_id_t)-1)

/* Union-Find structure with path compression and union by rank */
typedef struct unionfind {
    /* Parent pointers (parent[i] is the parent of element i) */
    uf_id_t* parent;
    
    /* Rank for union by rank heuristic */
    uint32_t* rank;
    
    /* Number of elements */
    size_t size;
    
    /* Capacity of the arrays */
    size_t capacity;
    
    /* Number of distinct sets */
    size_t set_count;
} unionfind_t;

/* Creation and destruction */
unionfind_t* unionfind_create(size_t initial_capacity);
void unionfind_destroy(unionfind_t* uf);

/* Add a new element - returns its ID */
uf_id_t unionfind_make_set(unionfind_t* uf);

/* Find the representative of a set (with path compression) */
uf_id_t unionfind_find(unionfind_t* uf, uf_id_t id);

/* Union two sets - returns the new representative */
uf_id_t unionfind_union(unionfind_t* uf, uf_id_t a, uf_id_t b);

/* Check if two elements are in the same set */
bool unionfind_equiv(unionfind_t* uf, uf_id_t a, uf_id_t b);

/* Get the number of elements */
size_t unionfind_size(const unionfind_t* uf);

/* Get the number of distinct sets */
size_t unionfind_set_count(const unionfind_t* uf);

/* Clear all elements */
void unionfind_clear(unionfind_t* uf);

/* Reserve capacity for at least n elements */
void unionfind_reserve(unionfind_t* uf, size_t n);

/* Check if an ID is valid */
bool unionfind_is_valid_id(const unionfind_t* uf, uf_id_t id);

/* String representation for debugging */
char* unionfind_to_string(const unionfind_t* uf);


/* Legacy API compatibility (eqx_ prefix) */
typedef unionfind_t eqx_unionfind_t;
typedef uf_id_t eqx_eclass_id_t;

#ifndef EQX_ECLASS_ID_INVALID
#define EQX_ECLASS_ID_INVALID ((eqx_eclass_id_t)-1)
#endif

eqx_unionfind_t* eqx_unionfind_create(size_t initial_capacity);
void eqx_unionfind_destroy(eqx_unionfind_t* uf);
eqx_eclass_id_t eqx_unionfind_make_set(eqx_unionfind_t* uf);
eqx_eclass_id_t eqx_unionfind_find(eqx_unionfind_t* uf, eqx_eclass_id_t id);
eqx_eclass_id_t eqx_unionfind_union(eqx_unionfind_t* uf, eqx_eclass_id_t id1, eqx_eclass_id_t id2);
bool eqx_unionfind_equiv(eqx_unionfind_t* uf, eqx_eclass_id_t id1, eqx_eclass_id_t id2);
size_t eqx_unionfind_size(const eqx_unionfind_t* uf);
size_t eqx_unionfind_num_sets(const eqx_unionfind_t* uf);

static inline bool eqx_unionfind_equivalent(eqx_unionfind_t* uf, eqx_eclass_id_t id1, eqx_eclass_id_t id2) {
    return eqx_unionfind_equiv(uf, id1, id2);
}

static inline size_t eqx_unionfind_count_sets(const eqx_unionfind_t* uf) {
    return eqx_unionfind_num_sets(uf);
}


#ifdef __cplusplus
}
#endif

#endif /* EQUINOX_UNIONFIND_H */
