#include "equinox/hashcons.h"
#include <stdlib.h>
#include <string.h>

typedef struct eqx_hashcons_entry {
    eqx_enode_t* node;
    eqx_eclass_id_t id;
    struct eqx_hashcons_entry* next;
} eqx_hashcons_entry_t;

struct eqx_hashcons {
    size_t bucket_count;
    size_t size;
    double load_factor;
    eqx_hashcons_entry_t** buckets;
};

struct eqx_hashcons_iter {
    const eqx_hashcons_t* hc;
    size_t bucket;
    const eqx_hashcons_entry_t* entry;
};

static uint32_t hash_node(const eqx_enode_t* node) {
    return eqx_enode_hash(node);
}

static bool entries_match(const eqx_hashcons_entry_t* entry, const eqx_enode_t* node) {
    return eqx_enode_equal(entry->node, node);
}

static bool ensure_capacity(eqx_hashcons_t* hc) {
    double ratio = (hc->bucket_count > 0) ? (double)hc->size / (double)hc->bucket_count : 0.0;
    if (ratio < hc->load_factor || hc->bucket_count == 0) return true;

    size_t new_buckets = hc->bucket_count * 2;
    eqx_hashcons_entry_t** next = (eqx_hashcons_entry_t**)calloc(new_buckets, sizeof(*next));
    if (!next) return false;

    for (size_t i = 0; i < hc->bucket_count; i++) {
        eqx_hashcons_entry_t* it = hc->buckets[i];
        while (it) {
            eqx_hashcons_entry_t* next_entry = it->next;
            uint32_t h = hash_node(it->node);
            size_t idx = h % new_buckets;
            it->next = next[idx];
            next[idx] = it;
            it = next_entry;
        }
    }

    free(hc->buckets);
    hc->buckets = next;
    hc->bucket_count = new_buckets;
    return true;
}

eqx_hashcons_t* eqx_hashcons_create(size_t initial_capacity, double load_factor) {
    if (initial_capacity == 0) initial_capacity = 16;
    if (load_factor <= 0.0) load_factor = 0.75;

    eqx_hashcons_t* hc = (eqx_hashcons_t*)calloc(1, sizeof(eqx_hashcons_t));
    if (!hc) return NULL;

    hc->bucket_count = initial_capacity;
    hc->size = 0;
    hc->load_factor = load_factor;
    hc->buckets = (eqx_hashcons_entry_t**)calloc(hc->bucket_count, sizeof(*hc->buckets));
    if (!hc->buckets) {
        free(hc);
        return NULL;
    }

    return hc;
}

void eqx_hashcons_destroy(eqx_hashcons_t* hc) {
    if (!hc) return;

    for (size_t i = 0; i < hc->bucket_count; i++) {
        eqx_hashcons_entry_t* it = hc->buckets[i];
        while (it) {
            eqx_hashcons_entry_t* next = it->next;
            free(it->node->children);
            free(it->node);
            free(it);
            it = next;
        }
    }

    free(hc->buckets);
    free(hc);
}

bool eqx_hashcons_lookup(const eqx_hashcons_t* hc, const eqx_enode_t* node, eqx_eclass_id_t* out_id) {
    if (!hc || !node) return false;
    if (hc->bucket_count == 0) return false;

    uint32_t h = hash_node(node);
    size_t idx = h % hc->bucket_count;
    for (eqx_hashcons_entry_t* it = hc->buckets[idx]; it; it = it->next) {
        if (entries_match(it, node)) {
            if (out_id) *out_id = it->id;
            return true;
        }
    }

    return false;
}

bool eqx_hashcons_contains(const eqx_hashcons_t* hc, const eqx_enode_t* node) {
    return eqx_hashcons_lookup(hc, node, NULL);
}

bool eqx_hashcons_insert(eqx_hashcons_t* hc, const eqx_enode_t* node, eqx_eclass_id_t eclass_id) {
    if (!hc || !node) return false;

    eqx_eclass_id_t existing;
    if (eqx_hashcons_lookup(hc, node, &existing)) {
        return false;
    }

    if (!ensure_capacity(hc)) {
        return false;
    }

    eqx_hashcons_entry_t* entry = (eqx_hashcons_entry_t*)calloc(1, sizeof(eqx_hashcons_entry_t));
    if (!entry) return false;

    size_t arity = eqx_enode_get_arity(node);
    eqx_enode_t* copy = (eqx_enode_t*)malloc(sizeof(eqx_enode_t));
    if (!copy) {
        free(entry);
        return false;
    }

    *copy = *node;
    if (node->children && arity > 0) {
        copy->children = (eqx_eclass_id_t*)malloc(arity * sizeof(eqx_eclass_id_t));
        if (!copy->children) {
            free(copy);
            free(entry);
            return false;
        }
        memcpy(copy->children, node->children, arity * sizeof(eqx_eclass_id_t));
    }

    entry->node = copy;
    entry->id = eclass_id;
    uint32_t h = hash_node(node);
    size_t idx = h % hc->bucket_count;
    entry->next = hc->buckets[idx];
    hc->buckets[idx] = entry;
    hc->size++;
    return true;
}

bool eqx_hashcons_remove(eqx_hashcons_t* hc, const eqx_enode_t* node) {
    if (!hc || !node || hc->bucket_count == 0) return false;

    uint32_t h = hash_node(node);
    size_t idx = h % hc->bucket_count;
    eqx_hashcons_entry_t** link = &hc->buckets[idx];
    while (*link) {
        eqx_hashcons_entry_t* entry = *link;
        if (entries_match(entry, node)) {
            *link = entry->next;
            free(entry->node->children);
            free(entry->node);
            free(entry);
            hc->size--;
            return true;
        }
        link = &entry->next;
    }
    return false;
}

void eqx_hashcons_update(eqx_hashcons_t* hc, const eqx_enode_t* node, eqx_eclass_id_t new_id) {
    if (!hc || !node) return;
    eqx_eclass_id_t current;
    if (eqx_hashcons_lookup(hc, node, &current)) {
        if (hc->bucket_count == 0) return;
        uint32_t h = hash_node(node);
        size_t idx = h % hc->bucket_count;
        for (eqx_hashcons_entry_t* it = hc->buckets[idx]; it; it = it->next) {
            if (entries_match(it, node)) {
                it->id = new_id;
                return;
            }
        }
    }
}

size_t eqx_hashcons_size(const eqx_hashcons_t* hc) {
    return hc ? hc->size : 0;
}

eqx_hashcons_iter_t eqx_hashcons_iter_begin(const eqx_hashcons_t* hc) {
    if (!hc) return NULL;

    struct eqx_hashcons_iter* it = (struct eqx_hashcons_iter*)malloc(sizeof(struct eqx_hashcons_iter));
    if (!it) return NULL;

    it->hc = hc;
    it->bucket = 0;
    it->entry = NULL;

    /* Find first bucket */
    for (size_t i = 0; i < hc->bucket_count; i++) {
        if (hc->buckets[i]) {
            it->bucket = i;
            it->entry = hc->buckets[i];
            break;
        }
    }

    if (!it->entry) {
        it->bucket = hc->bucket_count;
    }

    return it;
}

bool eqx_hashcons_iter_has_next(eqx_hashcons_iter_t iter) {
    return iter && iter->entry != NULL;
}

void eqx_hashcons_iter_next(eqx_hashcons_iter_t iter, eqx_enode_t** node, eqx_eclass_id_t* out_id) {
    if (!iter || !iter->entry) return;

    if (node) *node = iter->entry->node;
    if (out_id) *out_id = iter->entry->id;

    iter->entry = iter->entry->next;
    if (!iter->entry) {
        for (size_t i = iter->bucket + 1; i < iter->hc->bucket_count; i++) {
            if (iter->hc->buckets[i]) {
                iter->bucket = i;
                iter->entry = iter->hc->buckets[i];
                return;
            }
        }
        iter->bucket = iter->hc->bucket_count;
    }
}

void eqx_hashcons_iter_end(eqx_hashcons_iter_t iter) {
    free(iter);
}
