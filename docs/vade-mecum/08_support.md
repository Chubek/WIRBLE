# Chapter 7: Memory Management and Support Utilities

## Overview

WIRBLE provides a comprehensive set of support utilities for memory management, diagnostics, string handling, and data structures. These utilities form the foundation for all WIRBLE subsystems.

## Arena Allocator

### Design

The arena allocator provides fast bulk allocation with efficient deallocation:

- **Fast Allocation**: Bump-pointer allocation
- **Bulk Deallocation**: Free entire arena at once
- **No Individual Frees**: Reduces overhead
- **Block-Based**: Grows as needed

### wirble_arena Structure

```c
typedef struct wirble_arena {
    struct wirble_arena_block *current;
    struct wirble_arena_block *blocks;
    size_t defaultBlockSize;
    size_t totalReserved;
    size_t totalUsed;
} wirble_arena;
```

### Core Functions

#### Initialization

```c
void wirble_arena_init(wirble_arena *arena, size_t blockSize);
```

Initialize an arena with a default block size (typically 4KB or 64KB).

#### Allocation

```c
void *wirble_arena_alloc(wirble_arena *arena, size_t size);
void *wirble_arena_alloc_from(wirble_arena *arena, size_t size, size_t alignment);
void *wirble_arena_calloc_from(wirble_arena *arena, size_t count, size_t size, size_t alignment);
```

Allocate memory from the arena. `alloc_from` allows custom alignment.

#### String Duplication

```c
char *wirble_arena_strdup(wirble_arena *arena, const char *str);
```

Duplicate a string in the arena.

#### Statistics

```c
size_t wirble_arena_total_reserved(const wirble_arena *arena);
size_t wirble_arena_total_used(const wirble_arena *arena);
```

Query memory usage statistics.

#### Reset and Destroy

```c
void wirble_arena_reset(wirble_arena *arena);
void wirble_arena_destroy(wirble_arena *arena);
```

Reset reuses existing blocks; destroy frees all memory.

### Helper Functions

```c
size_t wirble_align_up(size_t size, size_t alignment);
size_t wirble_max_size(size_t a, size_t b);
```

### Example Usage

```c
wirble_arena arena;
wirble_arena_init(&arena, 4096);

// Allocate various objects
int *numbers = wirble_arena_alloc(&arena, sizeof(int) * 100);
char *name = wirble_arena_strdup(&arena, "example");
struct MyStruct *obj = wirble_arena_alloc(&arena, sizeof(struct MyStruct));

// Use allocated memory...

// Free everything at once
wirble_arena_destroy(&arena);
```

## String Pool

### Design

String interning for efficient string storage and comparison:

- **Deduplication**: Identical strings stored once
- **Fast Comparison**: Compare pointers instead of contents
- **Hash-Based**: Fast lookup
- **Arena-Backed**: Uses arena for storage

### wirble_strpool Structure

```c
typedef struct wirble_strpool {
    wirble_arena arena;
    struct wirble_strpool_entry **buckets;
    size_t bucketCount;
    size_t entryCount;
    size_t totalBytes;
} wirble_strpool;
```

### Core Functions

#### Initialization

```c
void wirble_strpool_init(wirble_strpool *pool, size_t initialBuckets);
```

#### Interning

```c
const char *wirble_strpool_intern(wirble_strpool *pool, const char *str);
const char *wirble_strpool_intern_with_length(wirble_strpool *pool, const char *str, size_t len);
const char *wirble_strpool_intern_from_pool(wirble_strpool *dest, wirble_strpool *src, const char *str);
```

Intern a string. Returns a pointer to the canonical copy.

#### Statistics

```c
size_t wirble_strpool_size(const wirble_strpool *pool);
```

Get number of unique strings.

#### Cleanup

```c
void wirble_strpool_destroy(wirble_strpool *pool);
```

#### Internal Functions

```c
uint32_t wirble_strpool_hash_bytes(const uint8_t *data, size_t len);
void wirble_strpool_grow(wirble_strpool *pool);
```

### Example Usage

```c
wirble_strpool pool;
wirble_strpool_init(&pool, 256);

// Intern strings
const char *s1 = wirble_strpool_intern(&pool, "hello");
const char *s2 = wirble_strpool_intern(&pool, "hello");
const char *s3 = wirble_strpool_intern(&pool, "world");

// Fast comparison: s1 == s2 (same pointer)
assert(s1 == s2);
assert(s1 != s3);

// Clean up
wirble_strpool_destroy(&pool);
```

## Diagnostics System

### Design

Comprehensive error and warning reporting with source locations:

- **Severity Levels**: Error, warning, note, info
- **Source Locations**: File, line, column tracking
- **Formatted Output**: Printf-style messages
- **Counting**: Track error/warning counts
- **Configurable Output**: Direct to any FILE*

### wirble_diag Structure

```c
typedef struct wirble_diag {
    FILE *stream;
    const char *sourceName;
    const char *sourceText;
    uint32_t errorCount;
    uint32_t warningCount;
} wirble_diag;
```

### Severity Levels

```c
typedef enum wirble_diag_severity {
    WIRBLE_DIAG_ERROR,
    WIRBLE_DIAG_WARNING,
    WIRBLE_DIAG_NOTE,
    WIRBLE_DIAG_INFO
} wirble_diag_severity;
```

### Core Functions

#### Initialization

```c
void wirble_diag_init(wirble_diag *diag);
```

#### Configuration

```c
void wirble_diag_set_stream(wirble_diag *diag, FILE *stream);
void wirble_diag_set_source(wirble_diag *diag, const char *name, const char *text);
```

#### Reporting

```c
void wirble_diag_report(wirble_diag *diag, wirble_diag_severity severity,
                        uint32_t line, uint32_t column, const char *message);

void wirble_diag_reportf(wirble_diag *diag, wirble_diag_severity severity,
                         uint32_t line, uint32_t column, const char *fmt, ...);

void wirble_diag_vreportf(wirble_diag *diag, wirble_diag_severity severity,
                          uint32_t line, uint32_t column, const char *fmt, va_list args);
```

#### Counting

```c
void wirble_diag_increment(wirble_diag *diag, wirble_diag_severity severity);
uint32_t wirble_diag_count(const wirble_diag *diag, wirble_diag_severity severity);
int wirble_diag_has_errors(const wirble_diag *diag);
```

#### Output

```c
void wirble_diag_emit(wirble_diag *diag, wirble_diag_severity severity,
                      uint32_t line, uint32_t column, const char *message);
```

#### Utilities

```c
const char *wirble_diag_severity_name(wirble_diag_severity severity);
FILE *wirble_diag_stream(const wirble_diag *diag);
```

### Example Usage

```c
wirble_diag diag;
wirble_diag_init(&diag);
wirble_diag_set_stream(&diag, stderr);
wirble_diag_set_source(&diag, "example.wil", sourceCode);

// Report errors
wirble_diag_reportf(&diag, WIRBLE_DIAG_ERROR, 10, 5,
                    "undefined variable '%s'", varName);

wirble_diag_reportf(&diag, WIRBLE_DIAG_WARNING, 15, 10,
                    "unused variable '%s'", varName);

// Check for errors
if (wirble_diag_has_errors(&diag)) {
    fprintf(stderr, "Compilation failed with %u errors\n",
            wirble_diag_count(&diag, WIRBLE_DIAG_ERROR));
    return -1;
}
```

## Byte Buffer

### Design

Dynamic byte buffer for building binary data:

- **Dynamic Growth**: Automatically resizes
- **Efficient Appending**: Amortized O(1)
- **Type-Safe Writes**: Functions for various types
- **Alignment Support**: Proper data alignment

### wirble_bytebuf Structure

```c
typedef struct wirble_bytebuf {
    uint8_t *data;
    size_t size;
    size_t capacity;
} wirble_bytebuf;
```

### Core Functions

#### Initialization

```c
void wirble_bytebuf_init(wirble_bytebuf *buf);
void wirble_bytebuf_init_with_capacity(wirble_bytebuf *buf, size_t capacity);
```

#### Writing

```c
void wirble_bytebuf_write_byte(wirble_bytebuf *buf, uint8_t byte);
void wirble_bytebuf_write_u16(wirble_bytebuf *buf, uint16_t value);
void wirble_bytebuf_write_u32(wirble_bytebuf *buf, uint32_t value);
void wirble_bytebuf_write_u64(wirble_bytebuf *buf, uint64_t value);
void wirble_bytebuf_write_bytes(wirble_bytebuf *buf, const void *data, size_t len);
```

#### Alignment

```c
void wirble_bytebuf_align(wirble_bytebuf *buf, size_t alignment);
```

#### Access

```c
uint8_t *wirble_bytebuf_data(const wirble_bytebuf *buf);
size_t wirble_bytebuf_size(const wirble_bytebuf *buf);
```

#### Cleanup

```c
void wirble_bytebuf_destroy(wirble_bytebuf *buf);
```

### Example Usage

```c
wirble_bytebuf buf;
wirble_bytebuf_init(&buf);

// Write header
wirble_bytebuf_write_u32(&buf, MAGIC_NUMBER);
wirble_bytebuf_write_u32(&buf, VERSION);

// Write data
wirble_bytebuf_write_bytes(&buf, data, dataLen);

// Align for next section
wirble_bytebuf_align(&buf, 16);

// Use buffer
fwrite(wirble_bytebuf_data(&buf), 1, wirble_bytebuf_size(&buf), file);

// Clean up
wirble_bytebuf_destroy(&buf);
```

## Hash Table

### Design

Generic hash table implementation:

- **Open Addressing**: Linear probing
- **Generic Keys/Values**: Void pointers
- **Custom Hash Functions**: User-provided
- **Dynamic Resizing**: Grows as needed

### wirble_hash_table Structure

```c
typedef struct wirble_hash_table {
    struct wirble_hash_entry *entries;
    size_t capacity;
    size_t count;
    uint32_t (*hashFunc)(const void *key);
    int (*compareFunc)(const void *a, const void *b);
} wirble_hash_table;
```

### Core Functions

#### Initialization

```c
void wirble_hash_table_init(wirble_hash_table *table,
                            uint32_t (*hashFunc)(const void *),
                            int (*compareFunc)(const void *, const void *));
```

#### Operations

```c
void wirble_hash_table_insert(wirble_hash_table *table, void *key, void *value);
void *wirble_hash_table_lookup(wirble_hash_table *table, const void *key);
int wirble_hash_table_remove(wirble_hash_table *table, const void *key);
int wirble_hash_table_contains(wirble_hash_table *table, const void *key);
```

#### Statistics

```c
size_t wirble_hash_table_size(const wirble_hash_table *table);
size_t wirble_hash_table_capacity(const wirble_hash_table *table);
```

#### Cleanup

```c
void wirble_hash_table_destroy(wirble_hash_table *table);
```

### Example Usage

```c
// Hash function for strings
uint32_t string_hash(const void *key) {
    return wirble_strpool_hash_bytes((const uint8_t *)key, strlen(key));
}

// Compare function for strings
int string_compare(const void *a, const void *b) {
    return strcmp((const char *)a, (const char *)b);
}

// Create table
wirble_hash_table table;
wirble_hash_table_init(&table, string_hash, string_compare);

// Insert
wirble_hash_table_insert(&table, "key1", value1);
wirble_hash_table_insert(&table, "key2", value2);

// Lookup
void *value = wirble_hash_table_lookup(&table, "key1");

// Remove
wirble_hash_table_remove(&table, "key2");

// Clean up
wirble_hash_table_destroy(&table);
```

## File Mapping

### Design

Memory-mapped file I/O for efficient file access:

- **Memory Mapping**: Map files into memory
- **Efficient Access**: No explicit read calls
- **Cross-Platform**: Works on Unix and Windows

### wirble_filemap Structure

```c
typedef struct wirble_filemap {
    void *data;
    size_t size;
    int fd;
    void *handle;  // Platform-specific
} wirble_filemap;
```

### Core Functions

#### Opening

```c
int wirble_filemap_open(wirble_filemap *map, const char *path);
```

#### Access

```c
void *wirble_filemap_data(const wirble_filemap *map);
size_t wirble_filemap_size(const wirble_filemap *map);
```

#### Closing

```c
void wirble_filemap_close(wirble_filemap *map);
```

### Example Usage

```c
wirble_filemap map;
if (wirble_filemap_open(&map, "data.bin") != 0) {
    fprintf(stderr, "Failed to open file\n");
    return -1;
}

// Access file data directly
const uint8_t *data = wirble_filemap_data(&map);
size_t size = wirble_filemap_size(&map);

// Process data...

// Close
wirble_filemap_close(&map);
```

## S-Expression Support

### Design

S-expression parsing and manipulation for configuration and rule files:

- **Parsing**: Text to S-expression trees
- **Construction**: Build S-expressions programmatically
- **Traversal**: Visit and query S-expressions
- **Memory Management**: Arena-backed

### Core Functions

S-expression support is provided through the `sexp_reader` module. See the symbol manifest for available functions.

## Best Practices

### Arena Allocator

1. **Choose Appropriate Block Size**: 4KB for small objects, 64KB for large
2. **Reset Instead of Destroy**: Reuse arenas when possible
3. **Avoid Mixing Lifetimes**: Use separate arenas for different lifetimes
4. **Profile Memory Usage**: Monitor total reserved vs. used

### String Pool

1. **Initialize with Reasonable Size**: Avoid early resizing
2. **Use for Identifiers**: Perfect for variable names, keywords
3. **Don't Intern Large Strings**: Not suitable for large text blocks
4. **Share Pools**: Use one pool per compilation unit

### Diagnostics

1. **Set Source Early**: Provide source context for better messages
2. **Use Appropriate Severity**: Don't overuse errors
3. **Include Location**: Always provide line/column when available
4. **Format Messages Clearly**: Use printf-style formatting

### Byte Buffer

1. **Pre-Allocate When Possible**: Use `init_with_capacity` if size is known
2. **Align Properly**: Use `align` for structured data
3. **Write in Order**: Sequential writes are most efficient

### Hash Table

1. **Choose Good Hash Functions**: Minimize collisions
2. **Size Appropriately**: Start with expected capacity
3. **Handle Collisions**: Ensure compare function is correct

## Memory Management Strategy

WIRBLE uses a layered memory management approach:

1. **Arena**: Fast allocation for temporary objects
2. **String Pool**: Deduplicated strings
3. **Hash Tables**: Efficient lookups
4. **Manual Management**: For long-lived objects

### Typical Pattern

```c
// Create arena for temporary allocations
wirble_arena arena;
wirble_arena_init(&arena, 4096);

// Create string pool for identifiers
wirble_strpool strings;
wirble_strpool_init(&strings, 256);

// Create diagnostics
wirble_diag diag;
wirble_diag_init(&diag);

// ... do work ...

// Clean up (order matters)
wirble_diag_destroy(&diag);
wirble_strpool_destroy(&strings);
wirble_arena_destroy(&arena);
```

## Performance Considerations

### Arena Allocator
- **Allocation**: O(1) amortized
- **Deallocation**: O(1) for entire arena
- **Memory Overhead**: One pointer per block

### String Pool
- **Interning**: O(1) average, O(n) worst case
- **Comparison**: O(1) (pointer comparison)
- **Memory Overhead**: Hash table + deduplicated strings

### Hash Table
- **Insert/Lookup/Remove**: O(1) average, O(n) worst case
- **Resize**: O(n)
- **Memory Overhead**: ~2x capacity for low load factor

## Conclusion

WIRBLE's support utilities provide efficient, well-tested implementations of common data structures and memory management patterns. Using these utilities ensures consistency across the codebase and leverages optimized implementations.
