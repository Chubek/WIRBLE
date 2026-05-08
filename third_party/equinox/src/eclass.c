#include <equinox/eclass.h>
#include <equinox/libutils.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ============================================================================
 * E-class Creation and Destruction
 * ========================================================================== */

eqx_eclass_t* eqx_eclass_create(eqx_eclass_id_t id) {
    eqx_eclass_t* eclass = (eqx_eclass_t*)malloc(sizeof(eqx_eclass_t));
    if (!eclass) return NULL;
    
    eclass->id = id;
    eclass->nodes = NULL;
    eclass->parents = NULL;
    eclass->data = NULL;
    
    return eclass;
}

void eqx_eclass_destroy(eqx_eclass_t* eclass) {
    if (!eclass) return;
    
    // Note: We don't free individual nodes here - they're managed by the egraph
    // We also don't free parent entries - they're managed separately
    
    free(eclass);
}

/* ============================================================================
 * E-class Accessors
 * ========================================================================== */

eqx_eclass_id_t eqx_eclass_get_id(const eqx_eclass_t* eclass) {
    assert(eclass != NULL);
    return eclass->id;
}

eqx_enode_t* eqx_eclass_get_nodes(const eqx_eclass_t* eclass) {
    assert(eclass != NULL);
    return eclass->nodes;
}

void* eqx_eclass_get_data(const eqx_eclass_t* eclass) {
    assert(eclass != NULL);
    return eclass->data;
}

void eqx_eclass_set_data(eqx_eclass_t* eclass, void* data) {
    assert(eclass != NULL);
    eclass->data = data;
}

/* ============================================================================
 * E-class Node Management
 * ========================================================================== */

void eqx_eclass_add_node(eqx_eclass_t* eclass, eqx_enode_t* node) {
    assert(eclass != NULL);
    assert(node != NULL);
    
    // Add node to the front of the linked list
    node->next_in_class = eclass->nodes;
    eclass->nodes = node;
    node->eclass = eclass->id;
}

bool eqx_eclass_contains_node(const eqx_eclass_t* eclass, const eqx_enode_t* node) {
    assert(eclass != NULL);
    assert(node != NULL);
    
    for (eqx_enode_t* current = eclass->nodes; current != NULL; current = current->next_in_class) {
        if (eqx_enode_equal(current, node)) {
            return true;
        }
    }
    
    return false;
}

size_t eqx_eclass_node_count(const eqx_eclass_t* eclass) {
    assert(eclass != NULL);
    
    size_t count = 0;
    for (eqx_enode_t* current = eclass->nodes; current != NULL; current = current->next_in_class) {
        count++;
    }
    
    return count;
}

/* ============================================================================
 * E-class Parent Management
 * ========================================================================== */

void eqx_eclass_add_parent(eqx_eclass_t* eclass, eqx_enode_t* parent) {
    assert(eclass != NULL);
    assert(parent != NULL);
    
    eqx_parent_entry_t* entry = (eqx_parent_entry_t*)malloc(sizeof(eqx_parent_entry_t));
    if (!entry) return;
    
    entry->parent = parent;
    entry->next = eclass->parents;
    eclass->parents = entry;
}

void eqx_eclass_remove_parent(eqx_eclass_t* eclass, eqx_enode_t* parent) {
    assert(eclass != NULL);
    assert(parent != NULL);
    
    eqx_parent_entry_t** current = &eclass->parents;
    
    while (*current != NULL) {
        if ((*current)->parent == parent) {
            eqx_parent_entry_t* to_remove = *current;
            *current = (*current)->next;
            free(to_remove);
            return;
        }
        current = &(*current)->next;
    }
}

eqx_parent_entry_t* eqx_eclass_get_parents(const eqx_eclass_t* eclass) {
    assert(eclass != NULL);
    return eclass->parents;
}

size_t eqx_eclass_parent_count(const eqx_eclass_t* eclass) {
    assert(eclass != NULL);
    
    size_t count = 0;
    for (eqx_parent_entry_t* current = eclass->parents; current != NULL; current = current->next) {
        count++;
    }
    
    return count;
}

/* ============================================================================
 * E-class Merging
 * ========================================================================== */

void eqx_eclass_merge_into(eqx_eclass_t* from, eqx_eclass_t* to) {
    assert(from != NULL);
    assert(to != NULL);
    
    // Move all nodes from 'from' to 'to'
    if (from->nodes != NULL) {
        // Find the end of the 'from' node list
        eqx_enode_t* last = from->nodes;
        while (last->next_in_class != NULL) {
            last->eclass = to->id;  // Update eclass ID
            last = last->next_in_class;
        }
        last->eclass = to->id;  // Update last node's eclass ID
        
        // Append 'to' nodes to the end of 'from' nodes
        last->next_in_class = to->nodes;
        to->nodes = from->nodes;
        from->nodes = NULL;
    }
    
    // Move all parents from 'from' to 'to'
    if (from->parents != NULL) {
        // Find the end of the 'from' parent list
        eqx_parent_entry_t* last_parent = from->parents;
        while (last_parent->next != NULL) {
            last_parent = last_parent->next;
        }
        
        // Append 'to' parents to the end of 'from' parents
        last_parent->next = to->parents;
        to->parents = from->parents;
        from->parents = NULL;
    }
}

/* ============================================================================
 * E-class Utilities
 * ========================================================================== */

eqx_enode_t* eqx_eclass_find_node(const eqx_eclass_t* eclass, eqx_symbol_t op, 
                                   size_t arity, const eqx_eclass_id_t* children) {
    assert(eclass != NULL);
    assert(arity == 0 || children != NULL);
    
    for (eqx_enode_t* node = eclass->nodes; node != NULL; node = node->next_in_class) {
        if (node->op != op || node->arity != arity) {
            continue;
        }
        
        bool match = true;
        for (size_t i = 0; i < arity; i++) {
            if (node->children[i] != children[i]) {
                match = false;
                break;
            }
        }
        
        if (match) {
            return node;
        }
    }
    
    return NULL;
}

void eqx_eclass_print(const eqx_eclass_t* eclass, FILE* out) {
    if (!eclass) {
        fprintf(out, "<null>\n");
        return;
    }
    
    fprintf(out, "E-class %lu:\n", (unsigned long)eclass->id);
    
    fprintf(out, "  Nodes (%zu):\n", eqx_eclass_node_count(eclass));
    for (eqx_enode_t* node = eclass->nodes; node != NULL; node = node->next_in_class) {
        fprintf(out, "    ");
        eqx_enode_print(node, out);
        fprintf(out, "\n");
    }
    
    fprintf(out, "  Parents (%zu):\n", eqx_eclass_parent_count(eclass));
    for (eqx_parent_entry_t* entry = eclass->parents; entry != NULL; entry = entry->next) {
        fprintf(out, "    ");
        eqx_enode_print(entry->parent, out);
        fprintf(out, "\n");
    }
}
