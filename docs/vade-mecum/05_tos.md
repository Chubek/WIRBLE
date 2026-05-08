# Chapter 5: TOS (Target Optimization System)

## Overview

TOS (Target Optimization System) is WIRBLE's target-specific optimization layer. It operates on machine code after instruction selection and register allocation, applying peephole optimizations, managing trace recording, and providing profile-guided optimization with persistent storage.

## Design Philosophy

TOS focuses on:

1. **Target-Specific Optimization**: Optimizations that exploit specific machine characteristics
2. **Peephole Optimization**: Local instruction pattern replacement
3. **Trace Recording**: Capture hot execution paths
4. **Trace Caching**: Persistent storage of optimized traces
5. **Profile-Guided Optimization**: Use runtime profiles to guide optimization
6. **LMDB Integration**: Persistent database for optimization data

## Core Types

### TOSTraceId
```c
typedef uint64_t TOSTraceId;
#define TOS_INVALID_TRACE ((TOSTraceId) -1)
```
Unique identifier for execution traces.

### TOSVersionId
```c
typedef uint32_t TOSVersionId;
#define TOS_INVALID_VERSION ((TOSVersionId) -1)
```
Version identifier for trace variants.

### TOSInstrId
```c
typedef uint32_t TOSInstrId;
#define TOS_INVALID_INSTR ((TOSInstrId) -1)
```
Instruction identifier within a trace.

### TOSOrderKind

Instruction ordering strategies:

```c
typedef enum TOSOrderKind {
    TOS_ORDER_INPUT = 0,           // Original input order
    TOS_ORDER_REVERSE_POSTORDER    // Reverse postorder traversal
} TOSOrderKind;
```

## TOS Program Structure

### TOSInst

Individual instruction in TOS representation:

```c
typedef struct TOSInst {
    TOSInstrId id;
    uint32_t blockId;
    uint32_t order;
    const struct MDSInstruction *desc;
    MALOperand *operands;
    uint32_t operandCount;
} TOSInst;
```

### TOSBlock

Basic block in TOS:

```c
typedef struct TOSBlock {
    uint32_t id;
    uint32_t layoutIndex;
    TOSInstrId firstInstr;
    uint32_t instrCount;
    uint32_t *successors;
    uint32_t successorCount;
} TOSBlock;
```

### TOSProgram

Complete TOS program:

```c
typedef struct TOSProgram {
    const struct MDSMachine *machine;
    TOSBlock *blocks;
    uint32_t blockCount;
    TOSInst *instructions;
    uint32_t instrCount;
    TOSOrderKind orderKind;
    int isFinalized;
} TOSProgram;
```

## Building TOS Programs

### From MDS

Convert MDS program to TOS:

```c
TOSProgram *tosBuildFromMDS(const struct MDSProgram *program);
```

### Finalization

Finalize program before optimization:

```c
int tosFinalizeProgram(TOSProgram *program);
```

### Validation

Validate TOS program structure:

```c
int tosValidateProgram(const TOSProgram *program);
```

### Cleanup

```c
void tosDestroyProgram(TOSProgram *program);
```

## TOS Context

### TOSContext Structure

Main handle for the optimization system:

```c
typedef struct TOSContext {
    MDB_env *lmdbEnv;              // LMDB environment
    MDB_dbi peepholeDB;            // Peephole optimization database
    MDB_dbi subInstrDB;            // Sub-instruction database
    MDB_dbi symbolDB;              // Symbol database
    MDB_dbi traceDB;               // Trace database
    MDB_dbi versionDB;             // Version database
    MDB_dbi profileDB;             // Profile database
    
    const struct MDSMachine *machine;
    struct TOSRewriteSystem *rewriteSys;
    struct TOSTraceRecorder *traceRecorder;
    struct TOSTraceCache *traceCache;
    struct TOSVersionManager *versionMgr;
    
    uint64_t peepholesApplied;
    uint64_t subInstrCreated;
    uint64_t tracesRecorded;
    uint64_t cacheHits;
    uint64_t cacheMisses;
} TOSContext;
```

### Creating Context

```c
TOSContext *tosContextCreate(const char *dbPath, 
                             const struct MDSMachine *machine);
```

### Destroying Context

```c
void tosContextDestroy(TOSContext *ctx);
```

## Peephole Optimization

### Peephole Patterns

Peephole optimizations match small instruction sequences and replace them with better alternatives:

```c
typedef struct TOSPeepholePattern {
    const char *name;
    TOSInst *pattern;              // Pattern to match
    uint32_t patternLength;
    TOSInst *replacement;          // Replacement instructions
    uint32_t replacementLength;
    int (*constraint)(TOSInst *insts, void *userData);
} TOSPeepholePattern;
```

### Common Peephole Patterns

#### Dead Store Elimination
```
mov rax, rbx
mov rax, rcx    →    mov rax, rcx
```

#### Redundant Move Elimination
```
mov rax, rax    →    (removed)
```

#### Strength Reduction
```
imul rax, 8     →    shl rax, 3
```

#### Algebraic Simplification
```
add rax, 0      →    (removed)
xor rax, rax    →    mov rax, 0  (if zero flag not needed)
```

### Applying Peephole Optimizations

```c
int tosApplyPeepholeOpts(TOSContext *ctx, TOSProgram *program);
```

### Adding Custom Peephole Patterns

```c
void tosAddPeepholePattern(TOSContext *ctx, TOSPeepholePattern *pattern);
```

### Peephole Database

Patterns are stored in LMDB for persistence:

```c
int tosSavePeepholePattern(TOSContext *ctx, TOSPeepholePattern *pattern);
TOSPeepholePattern *tosLoadPeepholePattern(TOSContext *ctx, const char *name);
```

## Trace Recording

### Trace Recorder

Records execution traces for hot path optimization:

```c
typedef struct TOSTraceRecorder {
    TOSContext *ctx;
    int isRecording;
    TOSTraceId currentTrace;
    TOSInst *recordedInsts;
    uint32_t recordedCount;
    uint32_t recordedCapacity;
} TOSTraceRecorder;
```

### Starting Recording

```c
TOSTraceId tosStartTraceRecording(TOSContext *ctx, uint64_t startPC);
```

### Recording Instructions

```c
void tosRecordInstruction(TOSContext *ctx, TOSInst *inst);
```

### Stopping Recording

```c
void tosStopTraceRecording(TOSContext *ctx);
```

### Trace Structure

```c
typedef struct TOSTrace {
    TOSTraceId id;
    uint64_t startPC;
    uint64_t endPC;
    TOSInst *instructions;
    uint32_t instructionCount;
    uint64_t executionCount;
    uint64_t lastExecuted;
} TOSTrace;
```

### Saving Traces

```c
int tosSaveTrace(TOSContext *ctx, TOSTrace *trace);
```

### Loading Traces

```c
TOSTrace *tosLoadTrace(TOSContext *ctx, TOSTraceId id);
TOSTrace *tosFindTraceByPC(TOSContext *ctx, uint64_t pc);
```

## Trace Caching

### Cache Structure

```c
typedef struct TOSTraceCache {
    TOSContext *ctx;
    
    // Cache statistics
    uint64_t hits;
    uint64_t misses;
    uint64_t evictions;
    
    // Cache policy
    uint32_t maxEntries;
    int evictionPolicy;            // LRU, LFU, etc.
} TOSTraceCache;
```

### Cache Lookup

```c
TOSTrace *tosCacheLookup(TOSContext *ctx, uint64_t pc);
```

### Cache Insertion

```c
void tosCacheInsert(TOSContext *ctx, TOSTrace *trace);
```

### Cache Eviction

```c
void tosCacheEvict(TOSContext *ctx, TOSTraceId id);
void tosCacheClear(TOSContext *ctx);
```

### Cache Statistics

```c
typedef struct TOSCacheStats {
    uint64_t hits;
    uint64_t misses;
    uint64_t evictions;
    uint32_t currentEntries;
    double hitRate;
} TOSCacheStats;

TOSCacheStats *tosGetCacheStats(TOSContext *ctx);
```

## Version Management

### Trace Versioning

Multiple optimized versions of the same trace:

```c
typedef struct TOSVersion {
    TOSVersionId id;
    TOSTraceId traceId;
    TOSInst *instructions;
    uint32_t instructionCount;
    
    // Optimization level
    int optLevel;
    
    // Performance metrics
    uint64_t executionCount;
    uint64_t totalCycles;
    double avgCycles;
} TOSVersion;
```

### Creating Versions

```c
TOSVersionId tosCreateVersion(TOSContext *ctx, TOSTraceId traceId, 
                              int optLevel);
```

### Selecting Best Version

```c
TOSVersion *tosSelectBestVersion(TOSContext *ctx, TOSTraceId traceId);
```

### Version Comparison

```c
int tosCompareVersions(TOSVersion *v1, TOSVersion *v2);
```

## Profile-Guided Optimization

### Profile Data

```c
typedef struct TOSProfile {
    uint64_t pc;
    uint64_t executionCount;
    uint64_t totalCycles;
    uint64_t branchTaken;
    uint64_t branchNotTaken;
} TOSProfile;
```

### Recording Profiles

```c
void tosRecordProfile(TOSContext *ctx, uint64_t pc, uint64_t cycles);
```

### Loading Profiles

```c
TOSProfile *tosLoadProfile(TOSContext *ctx, uint64_t pc);
```

### Applying Profile Data

```c
int tosApplyProfileGuidedOpts(TOSContext *ctx, TOSProgram *program);
```

Profile-guided optimizations:
- **Hot Path Optimization**: Focus on frequently executed code
- **Branch Prediction**: Reorder code based on branch probabilities
- **Inlining Decisions**: Inline based on call frequency
- **Register Allocation**: Prioritize hot variables

## Rewrite System Integration

TOS includes its own rewrite system for target-specific transformations:

```c
typedef struct TOSRewriteSystem {
    TOSContext *ctx;
    struct TOSRewriteRule *rules;
    uint32_t ruleCount;
} TOSRewriteSystem;
```

### Adding Rewrite Rules

```c
void tosAddRewriteRule(TOSContext *ctx, struct TOSRewriteRule *rule);
```

### Applying Rewrites

```c
int tosApplyRewrites(TOSContext *ctx, TOSProgram *program);
```

## Sub-Instruction Optimization

### Sub-Instruction Database

Store optimized instruction subsequences:

```c
typedef struct TOSSubInstr {
    uint32_t id;
    TOSInst *instructions;
    uint32_t instructionCount;
    uint64_t useCount;
} TOSSubInstr;
```

### Creating Sub-Instructions

```c
uint32_t tosCreateSubInstr(TOSContext *ctx, TOSInst *insts, uint32_t count);
```

### Using Sub-Instructions

```c
TOSSubInstr *tosLoadSubInstr(TOSContext *ctx, uint32_t id);
```

## LMDB Integration

### Database Schema

TOS uses LMDB with multiple databases:

- **peepholeDB**: Peephole optimization patterns
- **subInstrDB**: Sub-instruction sequences
- **symbolDB**: Symbol table
- **traceDB**: Recorded traces
- **versionDB**: Trace versions
- **profileDB**: Profile data

### Database Operations

```c
int tosDBPut(TOSContext *ctx, MDB_dbi dbi, const void *key, size_t keyLen,
             const void *value, size_t valueLen);

int tosDBGet(TOSContext *ctx, MDB_dbi dbi, const void *key, size_t keyLen,
             void **value, size_t *valueLen);

int tosDBDelete(TOSContext *ctx, MDB_dbi dbi, const void *key, size_t keyLen);
```

### Database Transactions

```c
MDB_txn *tosBeginTransaction(TOSContext *ctx, int readOnly);
int tosCommitTransaction(MDB_txn *txn);
void tosAbortTransaction(MDB_txn *txn);
```

## Example: Complete TOS Pipeline

```c
// Create TOS context with LMDB database
TOSContext *ctx = tosContextCreate("./tos.db", machine);

// Build TOS program from MDS
TOSProgram *program = tosBuildFromMDS(mdsProgram);

// Finalize program
tosFinalizeProgram(program);

// Validate
if (!tosValidateProgram(program)) {
    fprintf(stderr, "Invalid TOS program\n");
    return -1;
}

// Apply peephole optimizations
tosApplyPeepholeOpts(ctx, program);

// Apply profile-guided optimizations
tosApplyProfileGuidedOpts(ctx, program);

// Start trace recording for hot path
TOSTraceId traceId = tosStartTraceRecording(ctx, hotPathPC);

// ... execute code, recording instructions ...

// Stop recording
tosStopTraceRecording(ctx);

// Load and optimize trace
TOSTrace *trace = tosLoadTrace(ctx, traceId);
tosOptimizeTrace(ctx, trace);

// Create optimized version
TOSVersionId versionId = tosCreateVersion(ctx, traceId, 2);

// Cache the optimized trace
tosCacheInsert(ctx, trace);

// Get statistics
TOSCacheStats *stats = tosGetCacheStats(ctx);
printf("Cache hit rate: %.2f%%\n", stats->hitRate * 100);

// Clean up
tosDestroyProgram(program);
tosContextDestroy(ctx);
```

## Best Practices

1. **Use Persistent Storage**: Leverage LMDB for cross-session optimization
2. **Profile Before Optimizing**: Focus on hot paths
3. **Validate After Optimization**: Ensure correctness
4. **Monitor Cache Performance**: Adjust cache size based on hit rate
5. **Version Aggressively**: Keep multiple optimization levels
6. **Test Peephole Patterns**: Verify correctness on edge cases
7. **Limit Trace Length**: Keep traces manageable
8. **Prune Old Traces**: Remove rarely-used traces

## Performance Considerations

### Optimization Overhead

- **Peephole Optimization**: Low overhead, high benefit
- **Trace Recording**: Moderate overhead during recording
- **Profile Collection**: Minimal overhead
- **LMDB Operations**: Fast for reads, moderate for writes

### Tuning Parameters

```c
typedef struct TOSConfig {
    uint32_t maxTraceLength;
    uint32_t maxCacheEntries;
    int peepholePassCount;
    int enableProfileGuidedOpts;
    int enableTraceRecording;
    int enableVersioning;
} TOSConfig;

void tosSetConfig(TOSContext *ctx, TOSConfig *config);
```

## Integration with Code Generation

After TOS optimization, emit final machine code:

```c
// Optimize with TOS
tosApplyPeepholeOpts(ctx, program);

// Convert back to MDS for emission
MDSProgram *optimizedMDS = tosToMDS(program);

// Emit machine code
MDSCodeBuffer *code = mdsEmitCode(optimizedMDS);
```

## Debugging and Profiling

### Trace Visualization

```c
void tosPrintTrace(FILE *out, TOSTrace *trace);
void tosPrintVersion(FILE *out, TOSVersion *version);
```

### Statistics

```c
typedef struct TOSStats {
    uint64_t peepholesApplied;
    uint64_t subInstrCreated;
    uint64_t tracesRecorded;
    uint64_t versionsCreated;
    uint64_t cacheHits;
    uint64_t cacheMisses;
} TOSStats;

TOSStats *tosGetStats(TOSContext *ctx);
void tosPrintStats(FILE *out, TOSStats *stats);
```

## Conclusion

TOS provides the final optimization layer in WIRBLE's compilation pipeline, focusing on target-specific improvements and runtime adaptation. Its integration with LMDB enables persistent optimization data, making it ideal for JIT scenarios where optimization knowledge accumulates over time.
