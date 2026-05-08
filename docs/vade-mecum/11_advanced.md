# Chapter 10: Advanced Topics and Optimization

## Overview

This chapter covers advanced WIRBLE topics including optimization strategies, performance tuning, debugging techniques, and extending WIRBLE with custom components.

## Optimization Strategies

### Multi-Level Optimization

WIRBLE enables optimization at multiple IR levels:

1. **WIL Level**: High-level, target-independent optimizations
2. **MAL Level**: Machine-aware but target-independent optimizations
3. **MDS Level**: Target-specific instruction selection
4. **TOS Level**: Peephole and trace-based optimizations

### Optimization Pipeline

```c
// Complete optimization pipeline
WILGraph *wilGraph = parseToWIL(source);

// WIL optimizations
wilApplyConstantFolding(ctx, wilGraph);
wilApplyDeadCodeElimination(ctx, wilGraph);
wilApplyCommonSubexpressionElimination(ctx, wilGraph);
wilApplyLoopInvariantCodeMotion(ctx, wilGraph);

// Lower to MAL
MALModule *malModule = wilLowerToMAL(ctx, wilGraph);

// MAL optimizations
malApplyConstantPropagation(malModule);
malApplyCopyPropagation(malModule);
malApplyStrengthReduction(malModule);

// Instruction selection
MDSProgram *mdsProgram = mdsSelectInstructions(malModule, machine);

// Register allocation
mdsAllocateRegisters(mdsProgram, machine);

// TOS optimizations
TOSProgram *tosProgram = tosBuildFromMDS(mdsProgram);
tosApplyPeepholeOpts(ctx, tosProgram);
tosApplyProfileGuidedOpts(ctx, tosProgram);

// Code generation
MDSCodeBuffer *code = mdsEmitCode(mdsProgram);
```

## WIL Optimizations

### Constant Folding

Evaluate constant expressions at compile time:

```c
// Pattern: ADD(CONST(a), CONST(b)) → CONST(a + b)
WILPattern *lhs = wilPatNode(ctx, WIL_NODE_ADD,
    (WILPattern*[]){
        wilPatNode(ctx, WIL_NODE_CONST_INT, NULL, 0),
        wilPatNode(ctx, WIL_NODE_CONST_INT, NULL, 0)
    }, 2);

WILNode *foldAdd(WILContext *ctx, WILMatch *match, void *ud) {
    WILNode *left = wilMatchGetBinding(match, "left");
    WILNode *right = wilMatchGetBinding(match, "right");
    int64_t result = wilConstGetInt(left) + wilConstGetInt(right);
    return wilConstInt(ctx, result);
}
```

### Dead Code Elimination

Remove unused computations:

```c
void wilEliminateDeadCode(WILContext *ctx, WILGraph *graph) {
    // Mark live nodes (reachable from outputs)
    wilMarkLiveNodes(graph);
    
    // Remove unmarked nodes
    for (uint32_t i = 0; i < graph->nodeCount; i++) {
        WILNode *node = &graph->nodes[i];
        if (!wilNodeIsMarked(node)) {
            wilRemoveNode(ctx, graph, node);
        }
    }
}
```

### Common Subexpression Elimination

Eliminate redundant computations:

```c
void wilEliminateCommonSubexpressions(WILContext *ctx, WILGraph *graph) {
    wirble_hash_table seen;
    wirble_hash_table_init(&seen, wilNodeHash, wilNodeEquals);
    
    for (uint32_t i = 0; i < graph->nodeCount; i++) {
        WILNode *node = &graph->nodes[i];
        WILNode *existing = wirble_hash_table_lookup(&seen, node);
        
        if (existing) {
            // Replace uses of node with existing
            wilReplaceAllUsesWith(ctx, node, existing);
        } else {
            wirble_hash_table_insert(&seen, node, node);
        }
    }
    
    wirble_hash_table_destroy(&seen);
}
```

### Loop Optimizations

#### Loop Invariant Code Motion

Move loop-invariant computations outside loops:

```c
void wilHoistLoopInvariants(WILContext *ctx, WILGraph *graph) {
    // Find loops
    WILLoop **loops = wilFindLoops(graph);
    
    for (uint32_t i = 0; i < loopCount; i++) {
        WILLoop *loop = loops[i];
        
        // Find invariant nodes
        for (uint32_t j = 0; j < loop->nodeCount; j++) {
            WILNode *node = loop->nodes[j];
            
            if (wilIsLoopInvariant(loop, node)) {
                // Move to preheader
                wilMoveToPreheader(ctx, loop, node);
            }
        }
    }
}
```

#### Loop Unrolling

Replicate loop body to reduce overhead:

```c
void wilUnrollLoop(WILContext *ctx, WILLoop *loop, uint32_t factor) {
    WILNode *body = loop->body;
    
    for (uint32_t i = 1; i < factor; i++) {
        WILNode *copy = wilCloneSubgraph(ctx, body);
        wilAppendToLoop(ctx, loop, copy);
    }
    
    // Update loop counter
    wilUpdateLoopCounter(ctx, loop, factor);
}
```

## MAL Optimizations

### Constant Propagation

Propagate constant values through the program:

```c
void malPropagateConstants(MALFunction *func) {
    wirble_hash_table constants;
    wirble_hash_table_init(&constants, NULL, NULL);
    
    for (uint32_t i = 0; i < func->instrCount; i++) {
        MALInst *inst = &func->instructions[i];
        
        if (inst->opcode == MAL_OP_LOAD_IMM) {
            // Record constant
            wirble_hash_table_insert(&constants, 
                (void*)(uintptr_t)inst->dst, 
                (void*)(uintptr_t)inst->immediate);
        } else {
            // Try to fold with constants
            malTryFoldWithConstants(inst, &constants);
        }
    }
    
    wirble_hash_table_destroy(&constants);
}
```

### Copy Propagation

Replace copies with original values:

```c
void malPropagateCopies(MALFunction *func) {
    wirble_hash_table copies;
    wirble_hash_table_init(&copies, NULL, NULL);
    
    for (uint32_t i = 0; i < func->instrCount; i++) {
        MALInst *inst = &func->instructions[i];
        
        if (inst->opcode == MAL_OP_COPY) {
            wirble_hash_table_insert(&copies,
                (void*)(uintptr_t)inst->dst,
                (void*)(uintptr_t)inst->src);
        } else {
            // Replace operands with copy sources
            malReplaceOperandsWithCopies(inst, &copies);
        }
    }
    
    wirble_hash_table_destroy(&copies);
}
```

### Strength Reduction

Replace expensive operations with cheaper equivalents:

```c
void malReduceStrength(MALFunction *func) {
    for (uint32_t i = 0; i < func->instrCount; i++) {
        MALInst *inst = &func->instructions[i];
        
        // MUL by power of 2 → SHL
        if (inst->opcode == MAL_OP_MUL && malIsConstant(inst->src2)) {
            int64_t value = malGetConstant(inst->src2);
            if (isPowerOfTwo(value)) {
                inst->opcode = MAL_OP_SHL;
                inst->src2 = malMakeConstant(log2(value));
            }
        }
        
        // DIV by power of 2 → SHR
        if (inst->opcode == MAL_OP_UDIV && malIsConstant(inst->src2)) {
            int64_t value = malGetConstant(inst->src2);
            if (isPowerOfTwo(value)) {
                inst->opcode = MAL_OP_LSHR;
                inst->src2 = malMakeConstant(log2(value));
            }
        }
    }
}
```

## Register Allocation

### Linear Scan

Fast register allocation algorithm:

```c
void mdsLinearScanRegisterAllocation(MDSProgram *program) {
    // Compute live intervals
    LiveInterval *intervals = mdsComputeLiveIntervals(program);
    
    // Sort by start point
    qsort(intervals, intervalCount, sizeof(LiveInterval), compareIntervals);
    
    // Allocate registers
    MDSRegId *active = NULL;
    uint32_t activeCount = 0;
    
    for (uint32_t i = 0; i < intervalCount; i++) {
        LiveInterval *interval = &intervals[i];
        
        // Expire old intervals
        mdsExpireOldIntervals(interval, &active, &activeCount);
        
        // Try to allocate register
        MDSRegId reg = mdsAllocateRegister(program->machine);
        if (reg != MDS_INVALID_REG) {
            interval->reg = reg;
            mdsAddToActive(&active, &activeCount, interval);
        } else {
            // Spill
            mdsSpillInterval(program, interval);
        }
    }
}
```

### Graph Coloring

Higher-quality register allocation:

```c
void mdsGraphColoringRegisterAllocation(MDSProgram *program) {
    // Build interference graph
    InterferenceGraph *graph = mdsBuildInterferenceGraph(program);
    
    // Simplify: remove nodes with degree < K
    while (mdsHasSimplifiableNode(graph)) {
        VirtualReg *node = mdsSelectSimplifiableNode(graph);
        mdsRemoveNode(graph, node);
        mdsPushOnStack(node);
    }
    
    // Spill if necessary
    while (mdsHasNodes(graph)) {
        VirtualReg *node = mdsSelectSpillCandidate(graph);
        mdsRemoveNode(graph, node);
        mdsPushOnStack(node);
    }
    
    // Color: assign registers
    while (mdsStackNotEmpty()) {
        VirtualReg *node = mdsPopFromStack();
        MDSRegId reg = mdsSelectColor(graph, node);
        if (reg != MDS_INVALID_REG) {
            node->physicalReg = reg;
        } else {
            mdsSpillVirtualReg(program, node);
        }
    }
}
```

## Instruction Scheduling

### List Scheduling

Reorder instructions to minimize stalls:

```c
void mdsScheduleInstructions(MDSProgram *program) {
    for (uint32_t i = 0; i < program->blockCount; i++) {
        MDSBlock *block = &program->blocks[i];
        
        // Build dependency graph
        DependencyGraph *depGraph = mdsBuildDependencyGraph(block);
        
        // Initialize ready list
        InstructionList ready = mdsGetReadyInstructions(depGraph);
        
        // Schedule instructions
        InstructionList scheduled;
        uint32_t cycle = 0;
        
        while (!mdsIsEmpty(&ready) || mdsHasUnscheduled(depGraph)) {
            // Select instruction with highest priority
            MDSInst *inst = mdsSelectInstruction(&ready, cycle);
            
            if (inst) {
                // Schedule instruction
                mdsScheduleAt(&scheduled, inst, cycle);
                
                // Update ready list
                mdsUpdateReadyList(&ready, depGraph, inst);
                
                cycle += inst->latency;
            } else {
                cycle++;
            }
        }
        
        // Replace block instructions
        block->instructions = scheduled.instructions;
    }
}
```

## Profile-Guided Optimization

### Collecting Profiles

```c
typedef struct ProfileData {
    uint64_t *executionCounts;
    uint64_t *branchTaken;
    uint64_t *branchNotTaken;
} ProfileData;

void collectProfile(MDSProgram *program, ProfileData *profile) {
    // Instrument code
    mdsInstrumentForProfiling(program);
    
    // Run workload
    mdsExecute(program);
    
    // Collect data
    profile->executionCounts = mdsGetExecutionCounts(program);
    profile->branchTaken = mdsGetBranchTaken(program);
    profile->branchNotTaken = mdsGetBranchNotTaken(program);
}
```

### Applying Profiles

```c
void applyProfileGuidedOpts(MDSProgram *program, ProfileData *profile) {
    // Identify hot paths
    MDSBlock **hotBlocks = mdsIdentifyHotBlocks(program, profile);
    
    // Optimize hot paths
    for (uint32_t i = 0; i < hotBlockCount; i++) {
        mdsOptimizeBlock(program, hotBlocks[i]);
    }
    
    // Reorder blocks for better locality
    mdsReorderBlocks(program, profile);
    
    // Inline hot calls
    mdsInlineHotCalls(program, profile);
}
```

## JIT Compilation

### Trace Compilation

```c
void wvmCompileTrace(struct WVMState *state, WVMTrace *trace) {
    // Convert trace to MAL
    MALFunction *func = wvmTraceToMAL(trace);
    
    // Optimize
    malApplyOptimizations(func);
    
    // Select instructions
    MDSProgram *program = mdsSelectInstructions(func, state->machine);
    
    // Allocate registers
    mdsAllocateRegisters(program, state->machine);
    
    // Emit native code
    void *nativeCode = mdsEmitNativeCode(program);
    
    // Install trace
    trace->nativeCode = nativeCode;
    trace->isCompiled = 1;
}
```

### Guard Optimization

```c
void wvmOptimizeGuards(WVMTrace *trace) {
    // Remove redundant guards
    for (uint32_t i = 0; i < trace->guardCount; i++) {
        WVMGuard *guard = &trace->guards[i];
        
        if (wvmIsRedundantGuard(trace, guard)) {
            wvmRemoveGuard(trace, guard);
        }
    }
    
    // Hoist guards out of loops
    wvmHoistGuards(trace);
    
    // Merge compatible guards
    wvmMergeGuards(trace);
}
```

## Debugging Techniques

### IR Visualization

```c
void visualizeWIL(WILGraph *graph, const char *filename) {
    FILE *f = fopen(filename, "w");
    fprintf(f, "digraph WIL {\n");
    
    for (uint32_t i = 0; i < graph->nodeCount; i++) {
        WILNode *node = &graph->nodes[i];
        fprintf(f, "  n%u [label=\"%s\"];\n", 
                node->id, wilNodeKindName(node->kind));
        
        for (uint32_t j = 0; j < node->inputCount; j++) {
            WILNode *input = node->inputs[j];
            fprintf(f, "  n%u -> n%u;\n", input->id, node->id);
        }
    }
    
    fprintf(f, "}\n");
    fclose(f);
    
    // Generate image: dot -Tpng graph.dot -o graph.png
}
```

### Instruction Tracing

```c
void enableInstructionTracing(struct WVMState *state) {
    state->traceEnabled = 1;
    state->traceFile = fopen("trace.log", "w");
}

void traceInstruction(struct WVMState *state, WVMInstr instr) {
    if (state->traceEnabled) {
        fprintf(state->traceFile, "%08llx: ", (unsigned long long)instr);
        wvmDisassembleInstruction(state->traceFile, instr);
        fprintf(state->traceFile, "\n");
    }
}
```

### Memory Debugging

```c
void enableMemoryDebugging(wirble_arena *arena) {
    arena->debugMode = 1;
    arena->allocationLog = fopen("alloc.log", "w");
}

void *debugArenaAlloc(wirble_arena *arena, size_t size) {
    void *ptr = wirble_arena_alloc(arena, size);
    
    if (arena->debugMode) {
        fprintf(arena->allocationLog, "alloc %p %zu\n", ptr, size);
    }
    
    return ptr;
}
```

## Performance Profiling

### Timing Measurements

```c
typedef struct Timer {
    uint64_t start;
    uint64_t total;
    uint32_t count;
} Timer;

void timerStart(Timer *timer) {
    timer->start = getCurrentTimeMicros();
}

void timerStop(Timer *timer) {
    uint64_t elapsed = getCurrentTimeMicros() - timer->start;
    timer->total += elapsed;
    timer->count++;
}

double timerAverage(const Timer *timer) {
    return (double)timer->total / timer->count;
}
```

### Profiling Compilation Phases

```c
typedef struct CompilerProfile {
    Timer parsing;
    Timer wilGeneration;
    Timer wilOptimization;
    Timer malLowering;
    Timer malOptimization;
    Timer instructionSelection;
    Timer registerAllocation;
    Timer codeGeneration;
} CompilerProfile;

void profileCompilation(CompilerProfile *profile, const char *source) {
    timerStart(&profile->parsing);
    AST *ast = parse(source);
    timerStop(&profile->parsing);
    
    timerStart(&profile->wilGeneration);
    WILGraph *wil = astToWIL(ast);
    timerStop(&profile->wilGeneration);
    
    // ... rest of pipeline ...
    
    printProfile(profile);
}
```

## Custom Extensions

### Custom WIL Node Types

```c
typedef enum CustomNodeKind {
    CUSTOM_NODE_VECTOR_ADD = WIL_NODE__CUSTOM_START,
    CUSTOM_NODE_MATRIX_MUL,
    CUSTOM_NODE_FFT
} CustomNodeKind;

WILNode *wilCreateVectorAdd(WILContext *ctx, WILNode *a, WILNode *b) {
    WILNode *node = wilCreateNode(ctx, CUSTOM_NODE_VECTOR_ADD);
    wilAddInput(node, a);
    wilAddInput(node, b);
    return node;
}
```

### Custom Rewrite Rules

```c
void registerCustomRules(WILRewriteSystem *sys) {
    // Vector add fusion: VADD(VADD(a, b), c) → VADD3(a, b, c)
    WILPattern *lhs = wilPatNode(ctx, CUSTOM_NODE_VECTOR_ADD,
        (WILPattern*[]){
            wilPatNode(ctx, CUSTOM_NODE_VECTOR_ADD, NULL, 0),
            wilPatVar(ctx, "c")
        }, 2);
    
    WILRewriteRule *rule = wilRuleCreate(ctx, "vadd-fusion", 
                                         lhs, fuseVectorAdds, NULL);
    wilRewriteSystemAddRule(sys, rule);
}
```

### Custom Machine Instructions

```json
{
  "id": 1000,
  "mnemonic": "custom_op",
  "operands": [
    {"name": "dst", "type": "reg", "class": "gpr"},
    {"name": "src", "type": "reg", "class": "gpr"}
  ],
  "encoding": {
    "format": "custom",
    "bytes": [0xFF, 0x00]
  }
}
```

## Best Practices

### Optimization

1. **Profile First**: Measure before optimizing
2. **Optimize Hot Paths**: Focus on frequently executed code
3. **Validate After Optimization**: Ensure correctness
4. **Use Multiple Passes**: Iterate optimizations
5. **Balance Compile Time**: Don't over-optimize cold code

### Debugging

1. **Enable Assertions**: Catch bugs early
2. **Visualize IR**: Use graphical representations
3. **Trace Execution**: Log instruction execution
4. **Validate Frequently**: Check invariants often
5. **Use Sanitizers**: AddressSanitizer, UBSan, etc.

### Performance

1. **Use Arena Allocation**: Faster than malloc/free
2. **Minimize Allocations**: Reuse memory when possible
3. **Cache Friendly**: Improve data locality
4. **Batch Operations**: Reduce overhead
5. **Profile Regularly**: Identify bottlenecks

## Conclusion

WIRBLE's multi-level optimization framework enables sophisticated compiler optimizations while maintaining modularity and extensibility. By understanding these advanced techniques, you can build high-performance compilers tailored to your specific needs.
