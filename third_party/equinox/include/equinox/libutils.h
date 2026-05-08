/**
 * @file libutils.h
 * @brief Header-only utility library for Equinox e-graph
 * 
 * Provides hash functions, memory arenas, string buffers, I/O utilities,
 * string pools, Ethash DAGs, symbol tables, and more.
 * 
 * Dependencies:
 * - TLSF (Two-Level Segregate Fit) memory allocator
 * - UThash (hash tables, dynamic arrays, string buffers)
 * - libdag-ethash (Ethash DAG generation)
 */

#ifndef EQUINOX_LIBUTILS_H
#define EQUINOX_LIBUTILS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/* uthash - header-only hash table library */
#include "uthash.h"
#include "utstring.h"
#include "utarray.h"

/* TLSF - Two-Level Segregated Fit memory allocator */
#include "tlsf.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * Compiler-specific inline function macros
 * ======================================================================== */

#if defined(_MSC_VER)
    #define LIBUTIL_FUNC static __forceinline
#elif defined(__GNUC__) || defined(__clang__)
    #define LIBUTIL_FUNC static inline __attribute__((always_inline))
#else
    #define LIBUTIL_FUNC static inline
#endif

/* ========================================================================
 * Memory Arena using TLSF
 * ======================================================================== */

typedef struct {
    tlsf_t tlsf;
    pool_t pool;
    void* memory;
    size_t capacity;
    size_t used;
} libutil_arena_t;

/**
 * @brief Create a memory arena with specified capacity
 * @param capacity Size in bytes for the arena
 * @return Pointer to arena or NULL on failure
 */
LIBUTIL_FUNC libutil_arena_t* libutil_arena_create(size_t capacity) {
    libutil_arena_t* arena = (libutil_arena_t*)malloc(sizeof(libutil_arena_t));
    if (!arena) return NULL;
    
    arena->capacity = capacity;
    arena->used = 0;
    arena->memory = malloc(capacity);
    if (!arena->memory) {
        free(arena);
        return NULL;
    }
    
    arena->tlsf = tlsf_create_with_pool(arena->memory, capacity);
    if (!arena->tlsf) {
        free(arena->memory);
        free(arena);
        return NULL;
    }
    
    arena->pool = tlsf_get_pool(arena->tlsf);
    return arena;
}

/**
 * @brief Allocate memory from arena
 * @param arena Arena to allocate from
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory or NULL
 */
LIBUTIL_FUNC void* libutil_arena_alloc(libutil_arena_t* arena, size_t size) {
    if (!arena || !arena->tlsf) return NULL;
    void* ptr = tlsf_malloc(arena->tlsf, size);
    if (ptr) {
        arena->used += tlsf_block_size(ptr);
    }
    return ptr;
}

/**
 * @brief Reallocate memory in arena
 * @param arena Arena containing the memory
 * @param ptr Pointer to existing allocation
 * @param new_size New size in bytes
 * @return Pointer to reallocated memory or NULL
 */
LIBUTIL_FUNC void* libutil_arena_realloc(libutil_arena_t* arena, void* ptr, size_t new_size) {
    if (!arena || !arena->tlsf) return NULL;
    size_t old_size = ptr ? tlsf_block_size(ptr) : 0;
    void* new_ptr = tlsf_realloc(arena->tlsf, ptr, new_size);
    if (new_ptr) {
        arena->used = arena->used - old_size + tlsf_block_size(new_ptr);
    }
    return new_ptr;
}

/**
 * @brief Free memory in arena
 * @param arena Arena containing the memory
 * @param ptr Pointer to free
 */
LIBUTIL_FUNC void libutil_arena_free(libutil_arena_t* arena, void* ptr) {
    if (!arena || !arena->tlsf || !ptr) return;
    size_t size = tlsf_block_size(ptr);
    tlsf_free(arena->tlsf, ptr);
    arena->used -= size;
}

/**
 * @brief Destroy arena and free all memory
 * @param arena Arena to destroy
 */
LIBUTIL_FUNC void libutil_arena_destroy(libutil_arena_t* arena) {
    if (!arena) return;
    if (arena->tlsf) {
        tlsf_destroy(arena->tlsf);
    }
    if (arena->memory) {
        free(arena->memory);
    }
    free(arena);
}

/**
 * @brief Get arena memory usage statistics
 * @param arena Arena to query
 * @param used Output: bytes currently used
 * @param capacity Output: total capacity
 */
LIBUTIL_FUNC void libutil_arena_stats(libutil_arena_t* arena, size_t* used, size_t* capacity) {
    if (!arena) return;
    if (used) *used = arena->used;
    if (capacity) *capacity = arena->capacity;
}

/* ========================================================================
 * Hash Functions
 * ======================================================================== */

/**
 * @brief FNV-1a hash function for strings
 * @param str String to hash
 * @return 32-bit hash value
 */
LIBUTIL_FUNC uint32_t libutil_hash_fnv1a(const char* str) {
    if (!str) return 0;
    uint32_t hash = 2166136261u;
    while (*str) {
        hash ^= (uint8_t)(*str++);
        hash *= 16777619u;
    }
    return hash;
}

/**
 * @brief FNV-1a hash for byte buffer
 * @param data Buffer to hash
 * @param len Length in bytes
 * @return 32-bit hash value
 */
LIBUTIL_FUNC uint32_t libutil_hash_fnv1a_buf(const void* data, size_t len) {
    if (!data) return 0;
    uint32_t hash = 2166136261u;
    const uint8_t* bytes = (const uint8_t*)data;
    for (size_t i = 0; i < len; i++) {
        hash ^= bytes[i];
        hash *= 16777619u;
    }
    return hash;
}

/**
 * @brief MurmurHash3 32-bit hash
 * @param key Data to hash
 * @param len Length in bytes
 * @param seed Hash seed
 * @return 32-bit hash value
 */
LIBUTIL_FUNC uint32_t libutil_hash_murmur3(const void* key, size_t len, uint32_t seed) {
    const uint8_t* data = (const uint8_t*)key;
    const int nblocks = len / 4;
    uint32_t h1 = seed;
    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;
    
    /* Body */
    const uint32_t* blocks = (const uint32_t*)(data + nblocks * 4);
    for (int i = -nblocks; i; i++) {
        uint32_t k1 = blocks[i];
        k1 *= c1;
        k1 = (k1 << 15) | (k1 >> 17);
        k1 *= c2;
        h1 ^= k1;
        h1 = (h1 << 13) | (h1 >> 19);
        h1 = h1 * 5 + 0xe6546b64;
    }
    
    /* Tail */
    const uint8_t* tail = (const uint8_t*)(data + nblocks * 4);
    uint32_t k1 = 0;
    switch (len & 3) {
        case 3: k1 ^= tail[2] << 16;
        case 2: k1 ^= tail[1] << 8;
        case 1: k1 ^= tail[0];
                k1 *= c1; k1 = (k1 << 15) | (k1 >> 17); k1 *= c2; h1 ^= k1;
    }
    
    /* Finalization */
    h1 ^= len;
    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;
    
    return h1;
}

/* ========================================================================
 * String Pool (interned strings)
 * ======================================================================== */

typedef struct libutil_string_entry {
    char* str;
    uint32_t hash;
    UT_hash_handle hh;
} libutil_string_entry_t;

typedef struct {
    libutil_string_entry_t* table;
    libutil_arena_t* arena;
} libutil_string_pool_t;

/**
 * @brief Create a string pool
 * @param arena_capacity Initial arena capacity (0 for default 64KB)
 * @return Pointer to string pool or NULL
 */
LIBUTIL_FUNC libutil_string_pool_t* libutil_string_pool_create(size_t arena_capacity) {
    libutil_string_pool_t* pool = (libutil_string_pool_t*)malloc(sizeof(libutil_string_pool_t));
    if (!pool) return NULL;
    
    pool->table = NULL;
    pool->arena = libutil_arena_create(arena_capacity > 0 ? arena_capacity : 65536);
    if (!pool->arena) {
        free(pool);
        return NULL;
    }
    
    return pool;
}

/**
 * @brief Intern a string in the pool
 * @param pool String pool
 * @param str String to intern
 * @return Pointer to interned string or NULL
 */
LIBUTIL_FUNC const char* libutil_string_pool_intern(libutil_string_pool_t* pool, const char* str) {
    if (!pool || !str) return NULL;
    
    uint32_t hash = libutil_hash_fnv1a(str);
    libutil_string_entry_t* entry = NULL;
    
    HASH_FIND(hh, pool->table, str, strlen(str), entry);
    if (entry) {
        return entry->str;
    }
    
    /* Allocate new entry */
    entry = (libutil_string_entry_t*)libutil_arena_alloc(pool->arena, sizeof(libutil_string_entry_t));
    if (!entry) return NULL;
    
    size_t len = strlen(str);
    entry->str = (char*)libutil_arena_alloc(pool->arena, len + 1);
    if (!entry->str) return NULL;
    
    memcpy(entry->str, str, len + 1);
    entry->hash = hash;
    
    HASH_ADD_KEYPTR(hh, pool->table, entry->str, len, entry);
    return entry->str;
}

/**
 * @brief Check if string exists in pool
 * @param pool String pool
 * @param str String to find
 * @return Pointer to interned string or NULL if not found
 */
LIBUTIL_FUNC const char* libutil_string_pool_find(libutil_string_pool_t* pool, const char* str) {
    if (!pool || !str) return NULL;
    
    libutil_string_entry_t* entry = NULL;
    HASH_FIND(hh, pool->table, str, strlen(str), entry);
    return entry ? entry->str : NULL;
}

/**
 * @brief Get number of strings in pool
 * @param pool String pool
 * @return Number of interned strings
 */
LIBUTIL_FUNC size_t libutil_string_pool_count(libutil_string_pool_t* pool) {
    if (!pool) return 0;
    return HASH_COUNT(pool->table);
}

/**
 * @brief Destroy string pool
 * @param pool String pool to destroy
 */
LIBUTIL_FUNC void libutil_string_pool_destroy(libutil_string_pool_t* pool) {
    if (!pool) return;
    
    libutil_string_entry_t* entry, *tmp;
    HASH_ITER(hh, pool->table, entry, tmp) {
        HASH_DEL(pool->table, entry);
    }
    
    if (pool->arena) {
        libutil_arena_destroy(pool->arena);
    }
    free(pool);
}

/* ========================================================================
 * Symbol Table
 * ======================================================================== */

typedef struct libutil_symbol {
    const char* name;
    void* value;
    uint32_t hash;
    UT_hash_handle hh;
} libutil_symbol_t;

typedef struct {
    libutil_symbol_t* table;
    libutil_string_pool_t* strings;
} libutil_symbol_table_t;

/**
 * @brief Create a symbol table
 * @return Pointer to symbol table or NULL
 */
LIBUTIL_FUNC libutil_symbol_table_t* libutil_symbol_table_create(void) {
    libutil_symbol_table_t* symtab = (libutil_symbol_table_t*)malloc(sizeof(libutil_symbol_table_t));
    if (!symtab) return NULL;
    
    symtab->table = NULL;
    symtab->strings = libutil_string_pool_create(0);
    if (!symtab->strings) {
        free(symtab);
        return NULL;
    }
    
    return symtab;
}

/**
 * @brief Insert or update symbol
 * @param symtab Symbol table
 * @param name Symbol name
 * @param value Symbol value
 * @return true on success
 */
LIBUTIL_FUNC bool libutil_symbol_table_set(libutil_symbol_table_t* symtab, const char* name, void* value) {
    if (!symtab || !name) return false;
    
    const char* interned = libutil_string_pool_intern(symtab->strings, name);
    if (!interned) return false;
    
    libutil_symbol_t* sym = NULL;
    HASH_FIND(hh, symtab->table, interned, strlen(interned), sym);
    
    if (sym) {
        sym->value = value;
    } else {
        sym = (libutil_symbol_t*)malloc(sizeof(libutil_symbol_t));
        if (!sym) return false;
        
        sym->name = interned;
        sym->value = value;
        sym->hash = libutil_hash_fnv1a(interned);
        
        HASH_ADD_KEYPTR(hh, symtab->table, sym->name, strlen(sym->name), sym);
    }
    
    return true;
}

/**
 * @brief Lookup symbol value
 * @param symtab Symbol table
 * @param name Symbol name
 * @param value Output: symbol value
 * @return true if found
 */
LIBUTIL_FUNC bool libutil_symbol_table_get(libutil_symbol_table_t* symtab, const char* name, void** value) {
    if (!symtab || !name) return false;
    
    libutil_symbol_t* sym = NULL;
    HASH_FIND(hh, symtab->table, name, strlen(name), sym);
    
    if (sym) {
        if (value) *value = sym->value;
        return true;
    }
    return false;
}

/**
 * @brief Remove symbol from table
 * @param symtab Symbol table
 * @param name Symbol name
 * @return true if removed
 */
LIBUTIL_FUNC bool libutil_symbol_table_remove(libutil_symbol_table_t* symtab, const char* name) {
    if (!symtab || !name) return false;
    
    libutil_symbol_t* sym = NULL;
    HASH_FIND(hh, symtab->table, name, strlen(name), sym);
    
    if (sym) {
        HASH_DEL(symtab->table, sym);
        free(sym);
        return true;
    }
    return false;
}

/**
 * @brief Get number of symbols
 * @param symtab Symbol table
 * @return Number of symbols
 */
LIBUTIL_FUNC size_t libutil_symbol_table_count(libutil_symbol_table_t* symtab) {
    if (!symtab) return 0;
    return HASH_COUNT(symtab->table);
}

/**
 * @brief Destroy symbol table
 * @param symtab Symbol table to destroy
 */
LIBUTIL_FUNC void libutil_symbol_table_destroy(libutil_symbol_table_t* symtab) {
    if (!symtab) return;
    
    libutil_symbol_t* sym, *tmp;
    HASH_ITER(hh, symtab->table, sym, tmp) {
        HASH_DEL(symtab->table, sym);
        free(sym);
    }
    
    if (symtab->strings) {
        libutil_string_pool_destroy(symtab->strings);
    }
    free(symtab);
}

/* ========================================================================
 * Dynamic String Buffer (wrapper around utstring)
 * ======================================================================== */

typedef UT_string libutil_strbuf_t;

/**
 * @brief Create a new string buffer
 * @return Pointer to string buffer or NULL
 */
LIBUTIL_FUNC libutil_strbuf_t* libutil_strbuf_create(void) {
    UT_string* s;
    utstring_new(s);
    return s;
}

/**
 * @brief Append string to buffer
 * @param buf String buffer
 * @param str String to append
 */
LIBUTIL_FUNC void libutil_strbuf_append(libutil_strbuf_t* buf, const char* str) {
    if (buf && str) utstring_printf(buf, "%s", str);
}

/**
 * @brief Append formatted string to buffer
 * @param buf String buffer
 * @param fmt Format string
 * @param ... Format arguments
 */
LIBUTIL_FUNC void libutil_strbuf_appendf(libutil_strbuf_t* buf, const char* fmt, ...) {
    if (!buf || !fmt) return;
    va_list ap;
    va_start(ap, fmt);
    utstring_printf_va(buf, fmt, ap);
    va_end(ap);
}

/**
 * @brief Get C string from buffer
 * @param buf String buffer
 * @return Pointer to null-terminated string
 */
LIBUTIL_FUNC const char* libutil_strbuf_cstr(libutil_strbuf_t* buf) {
    return buf ? utstring_body(buf) : NULL;
}

/**
 * @brief Get buffer length
 * @param buf String buffer
 * @return Length in bytes
 */
LIBUTIL_FUNC size_t libutil_strbuf_len(libutil_strbuf_t* buf) {
    return buf ? utstring_len(buf) : 0;
}

/**
 * @brief Clear buffer contents
 * @param buf String buffer
 */
LIBUTIL_FUNC void libutil_strbuf_clear(libutil_strbuf_t* buf) {
    if (buf) utstring_clear(buf);
}

/**
 * @brief Destroy string buffer
 * @param buf String buffer to destroy
 */
LIBUTIL_FUNC void libutil_strbuf_destroy(libutil_strbuf_t* buf) {
    if (buf) utstring_free(buf);
}

/* ========================================================================
 * I/O Utilities
 * ======================================================================== */

/**
 * @brief Read entire file into memory
 * @param filename Path to file
 * @param size Output: file size in bytes
 * @return Pointer to file contents (caller must free) or NULL
 */
LIBUTIL_FUNC char* libutil_read_file(const char* filename, size_t* size) {
    if (!filename) return NULL;
    
    FILE* f = fopen(filename, "rb");
    if (!f) return NULL;
    
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (fsize < 0) {
        fclose(f);
        return NULL;
    }
    
    char* buffer = (char*)malloc(fsize + 1);
    if (!buffer) {
        fclose(f);
        return NULL;
    }
    
    size_t read = fread(buffer, 1, fsize, f);
    fclose(f);
    
    buffer[read] = '\0';
    if (size) *size = read;
    
    return buffer;
}

/**
 * @brief Write buffer to file
 * @param filename Path to file
 * @param data Data to write
 * @param size Size in bytes
 * @return true on success
 */
LIBUTIL_FUNC bool libutil_write_file(const char* filename, const void* data, size_t size) {
    if (!filename || !data) return false;
    
    FILE* f = fopen(filename, "wb");
    if (!f) return false;
    
    size_t written = fwrite(data, 1, size, f);
    fclose(f);
    
    return written == size;
}

/**
 * @brief Check if file exists
 * @param filename Path to file
 * @return true if file exists
 */
LIBUTIL_FUNC bool libutil_file_exists(const char* filename) {
    if (!filename) return false;
    FILE* f = fopen(filename, "r");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

/* NOTE: libdag-ethash is vendored with a lower-level C API (`hashimoto*` and DAG/cache
 * helpers), which doesn't match this project's previous ethash object-style wrapper. The
 * ethash helpers here are intentionally omitted until a compatibility layer is reintroduced.
 */

#ifdef __cplusplus
}
#endif

#endif /* EQUINOX_LIBUTILS_H */
