# Chapter 11: Examples and Use Cases

## Overview

This chapter provides complete, practical examples of using WIRBLE for various compiler and runtime scenarios. Each example demonstrates key concepts and best practices.

## Example 1: Simple Expression Compiler

### Goal

Compile arithmetic expressions to x86_64 machine code.

### Input Language

```
expr := number
      | expr '+' expr
      | expr '*' expr
      | '(' expr ')'
```

### Implementation

```c
#include <wirble/wirble-wil.h>
#include <wirble/wirble-mal.h>
#include <wirble/wirble-mds.h>
#include <wirble/wirble-common.h>

typedef struct Expr {
    enum { EXPR_NUM, EXPR_ADD, EXPR_MUL } kind;
    union {
        int64_t number;
        struct { struct Expr *left, *right; } binary;
    };
} Expr;

WILNode *compileExpr(WILContext *ctx, Expr *expr) {
    switch (expr->kind) {
        case EXPR_NUM:
            return wilConstInt(ctx, expr->number);
        
        case EXPR_ADD: {
            WILNode *left = compileExpr(ctx, expr->binary.left);
            WILNode *right = compileExpr(ctx, expr->binary.right);
            return wilAdd(ctx, left, right);
        }
        
        case EXPR_MUL: {
            WILNode *left = compileExpr(ctx, expr->binary.left);
            WILNode *right = compileExpr(ctx, expr->binary.right);
            return wilMul(ctx, left, right);
        }
    }
    return NULL;
}

int main(int argc, char **argv) {
    // Parse expression
    Expr *expr = parseExpression(argv[1]);
    
    // Create WIL context
    WILContext *ctx = wilContextCreate();
    
    // Build WIL graph
    WILGraph *graph = wilGraphCreate(ctx);
    WILNode *start = wilStart(ctx);
    WILNode *result = compileExpr(ctx, expr);
    WILNode *ret = wilReturn(ctx, start, result);
    
    // Validate
    if (!wilValidateGraph(ctx, graph)) {
        fprintf(stderr, "Invalid WIL graph\n");
        return 1;
    }
    
    // Lower to MAL
    MALModule *module = wilLowerToMAL(ctx, graph);
    
    // Load machine description
    MDSMachine *machine = mdsLoadMachineJSON("machines/x86_64.json");
    
    // Select instructions
    MDSProgram *program = mdsSelectInstructions(module, machine);
    
    // Allocate registers
    mdsAllocateRegisters(program, machine);
    
    // Emit code
    MDSCodeBuffer *code = mdsEmitCode(program);
    
    // Write to file
    FILE *out = fopen("output.bin", "wb");
    fwrite(code->data, 1, code->size, out);
    fclose(out);
    
    printf("Generated %zu bytes of code\n", code->size);
    
    // Cleanup
    mdsCodeBufferDestroy(code);
    mdsProgramDestroy(program);
    mdsMachineDestroy(machine);
    malModuleDestroy(module);
    wilContextDestroy(ctx);
    
    return 0;
}
```

## Example 2: Stack-Based VM

### Goal

Implement a simple stack-based virtual machine using WVM.

### Bytecode Format

```
PUSH <value>    // Push value onto stack
ADD             // Pop two values, push sum
MUL             // Pop two values, push product
PRINT           // Pop and print value
HALT            // Stop execution
```

### Implementation

```c
#include <wirble/wirble-wvm.h>

typedef enum {
    OP_PUSH,
    OP_ADD,
    OP_MUL,
    OP_PRINT,
    OP_HALT
} Opcode;

void compileToWVM(uint8_t *bytecode, size_t len, WVMChunk *chunk) {
    WVMReg sp = 0;  // Stack pointer (register)
    
    for (size_t i = 0; i < len; ) {
        Opcode op = bytecode[i++];
        
        switch (op) {
            case OP_PUSH: {
                int32_t value = *(int32_t*)&bytecode[i];
                i += 4;
                wvmChunkEmitOpR(chunk, WVM_OP_LOAD_IMM, sp);
                wvmChunkEmit(chunk, value);
                sp++;
                break;
            }
            
            case OP_ADD: {
                sp--;
                wvmChunkEmitOpRRR(chunk, WVM_OP_ADD, sp-1, sp-1, sp);
                break;
            }
            
            case OP_MUL: {
                sp--;
                wvmChunkEmitOpRRR(chunk, WVM_OP_MUL, sp-1, sp-1, sp);
                break;
            }
            
            case OP_PRINT: {
                sp--;
                // Call print function
                wvmChunkEmitOpR(chunk, WVM_OP_CALL, sp);
                uint32_t printIdx = wvmChunkAddString(chunk, "print");
                wvmChunkEmit(chunk, printIdx);
                break;
            }
            
            case OP_HALT:
                wvmChunkEmitOp(chunk, WVM_OP_HALT);
                break;
        }
    }
}

int main(void) {
    // Create VM state
    struct WVMState *state = wvmStateCreate();
    
    // Register print function
    wvmRegisterCFunction(state, "print", myPrintFunction);
    
    // Create chunk
    WVMChunk *chunk = wvmChunkCreate();
    
    // Example bytecode: PUSH 10, PUSH 20, ADD, PRINT, HALT
    uint8_t bytecode[] = {
        OP_PUSH, 10, 0, 0, 0,
        OP_PUSH, 20, 0, 0, 0,
        OP_ADD,
        OP_PRINT,
        OP_HALT
    };
    
    // Compile
    compileToWVM(bytecode, sizeof(bytecode), chunk);
    
    // Execute
    wvmExecute(state, chunk);
    
    // Cleanup
    wvmChunkDestroy(chunk);
    wvmStateDestroy(state);
    
    return 0;
}
```

## Example 3: JIT Compiler for Loop

### Goal

JIT-compile a simple loop with runtime specialization.

### Source Code

```c
// Compute sum of array
int sum(int *arr, int n) {
    int total = 0;
    for (int i = 0; i < n; i++) {
        total += arr[i];
    }
    return total;
}
```

### Implementation

```c
#include <wirble/wirble-wil.h>
#include <wirble/wirble-mal.h>
#include <wirble/wirble-mds.h>
#include <wirble/wirble-tos.h>

typedef int (*SumFunc)(int*, int);

SumFunc jitCompileSum(void) {
    // Create WIL context
    WILContext *ctx = wilContextCreate();
    WILGraph *graph = wilGraphCreate(ctx);
    
    // Function parameters
    WILNode *start = wilStart(ctx);
    WILNode *arr = wilParam(ctx, 0);    // int *arr
    WILNode *n = wilParam(ctx, 1);      // int n
    
    // Initialize total = 0
    WILNode *zero = wilConstInt(ctx, 0);
    
    // Create loop
    WILNode *loop = wilLoop(ctx, start);
    
    // Loop variables: i and total
    WILNode *i = wilPhi(ctx, loop, (WILNode*[]){zero, NULL}, 2);
    WILNode *total = wilPhi(ctx, loop, (WILNode*[]){zero, NULL}, 2);
    
    // Load arr[i]
    WILNode *offset = wilMul(ctx, i, wilConstInt(ctx, 4));
    WILNode *addr = wilAdd(ctx, arr, offset);
    WILNode *value = wilLoad(ctx, addr, loop);
    
    // total += arr[i]
    WILNode *newTotal = wilAdd(ctx, total, value);
    
    // i++
    WILNode *one = wilConstInt(ctx, 1);
    WILNode *newI = wilAdd(ctx, i, one);
    
    // Update PHI nodes
    wilPhiSetInput(i, 1, newI);
    wilPhiSetInput(total, 1, newTotal);
    
    // Loop condition: i < n
    WILNode *cond = wilLt(ctx, newI, n);
    WILNode *ifNode = wilIf(ctx, loop, cond);
    
    // Back edge
    WILNode *continueProj = wilProj(ctx, ifNode, 0);
    wilLoopSetBackEdge(ctx, loop, continueProj);
    
    // Exit
    WILNode *exitProj = wilProj(ctx, ifNode, 1);
    WILNode *ret = wilReturn(ctx, exitProj, newTotal);
    
    // Optimize
    WILRewriteSystem *sys = wilRewriteSystemCreate(ctx);
    wilRewriteSystemAddRule(sys, constantFoldingRule);
    wilRewriteSystemAddRule(sys, strengthReductionRule);
    wilRewriteExhaustive(ctx, sys, graph);
    
    // Lower to MAL
    MALModule *module = wilLowerToMAL(ctx, graph);
    
    // Load machine
    MDSMachine *machine = mdsGetX86_64Machine();
    
    // Select instructions
    MDSProgram *program = mdsSelectInstructions(module, machine);
    
    // Allocate registers
    mdsAllocateRegisters(program, machine);
    
    // Apply TOS optimizations
    TOSContext *tosCtx = tosContextCreate("./jit-cache.db", machine);
    TOSProgram *tosProgram = tosBuildFromMDS(program);
    tosApplyPeepholeOpts(tosCtx, tosProgram);
    
    // Emit code
    MDSCodeBuffer *code = mdsEmitCode(program);
    
    // Make executable
    void *execMem = allocExecutableMemory(code->size);
    memcpy(execMem, code->data, code->size);
    makeExecutable(execMem, code->size);
    
    // Cast to function pointer
    SumFunc func = (SumFunc)execMem;
    
    // Cleanup (keep execMem)
    mdsCodeBufferDestroy(code);
    tosContextDestroy(tosCtx);
    mdsProgramDestroy(program);
    malModuleDestroy(module);
    wilContextDestroy(ctx);
    
    return func;
}

int main(void) {
    // JIT compile
    SumFunc sum = jitCompileSum();
    
    // Test
    int arr[] = {1, 2, 3, 4, 5};
    int result = sum(arr, 5);
    printf("Sum: %d\n", result);  // Output: 15
    
    return 0;
}
```

## Example 4: DSL Compiler

### Goal

Compile a domain-specific language for image processing.

### DSL Syntax

```
image = load("input.png")
blurred = blur(image, radius=5)
edges = sobel(blurred)
save(edges, "output.png")
```

### Implementation

```c
#include <wirble/wirble-wil.h>
#include <wirble/wirble-mal.h>
#include <wirble/wirble-mds.h>

typedef struct ImageOp {
    enum { OP_LOAD, OP_BLUR, OP_SOBEL, OP_SAVE } kind;
    union {
        const char *filename;
        struct { struct ImageOp *input; int radius; } blur;
        struct { struct ImageOp *input; } sobel;
        struct { struct ImageOp *input; const char *filename; } save;
    };
} ImageOp;

WILNode *compileImageOp(WILContext *ctx, ImageOp *op) {
    switch (op->kind) {
        case OP_LOAD: {
            // Call runtime load function
            WILNode *filename = wilConstString(ctx, op->filename);
            return wilCall(ctx, "image_load", (WILNode*[]){filename}, 1);
        }
        
        case OP_BLUR: {
            WILNode *input = compileImageOp(ctx, op->blur.input);
            WILNode *radius = wilConstInt(ctx, op->blur.radius);
            return wilCall(ctx, "image_blur", (WILNode*[]){input, radius}, 2);
        }
        
        case OP_SOBEL: {
            WILNode *input = compileImageOp(ctx, op->sobel.input);
            return wilCall(ctx, "image_sobel", (WILNode*[]){input}, 1);
        }
        
        case OP_SAVE: {
            WILNode *input = compileImageOp(ctx, op->save.input);
            WILNode *filename = wilConstString(ctx, op->save.filename);
            return wilCall(ctx, "image_save", (WILNode*[]){input, filename}, 2);
        }
    }
    return NULL;
}

// Optimize: fuse operations
void optimizeImagePipeline(WILContext *ctx, WILGraph *graph) {
    // Pattern: SOBEL(BLUR(x, r)) → SOBEL_BLUR(x, r)
    WILPattern *lhs = wilPatNode(ctx, WIL_NODE_CALL,
        (WILPattern*[]){
            wilPatConst(ctx, "image_sobel"),
            wilPatNode(ctx, WIL_NODE_CALL,
                (WILPattern*[]){
                    wilPatConst(ctx, "image_blur"),
                    wilPatVar(ctx, "input"),
                    wilPatVar(ctx, "radius")
                }, 3)
        }, 2);
    
    WILRewriteRule *rule = wilRuleCreate(ctx, "fuse-sobel-blur", 
                                         lhs, fuseSobelBlur, NULL);
    
    WILRewriteSystem *sys = wilRewriteSystemCreate(ctx);
    wilRewriteSystemAddRule(sys, rule);
    wilRewriteExhaustive(ctx, sys, graph);
}
```

## Example 5: Embedding WVM

### Goal

Embed WVM in a host application for scripting.

### Host Application

```c
#include <wirble/wirble-wvm.h>

// Host function callable from scripts
WVMValue hostPrint(struct WVMState *state, WVMValue *args, uint32_t argCount) {
    for (uint32_t i = 0; i < argCount; i++) {
        if (wvmIsNumber(args[i])) {
            printf("%g ", wvmToNumber(args[i]));
        } else if (wvmIsString(args[i])) {
            printf("%s ", wvmToString(args[i]));
        }
    }
    printf("\n");
    return wvmNil();
}

WVMValue hostGetTime(struct WVMState *state, WVMValue *args, uint32_t argCount) {
    return wvmNumber(getCurrentTime());
}

int main(void) {
    // Create VM
    struct WVMState *state = wvmStateCreate();
    
    // Register host functions
    wvmRegisterCFunction(state, "print", hostPrint);
    wvmRegisterCFunction(state, "getTime", hostGetTime);
    
    // Load and execute script
    WVMChunk *chunk = wvmLoadChunk("script.wvm");
    if (!chunk) {
        fprintf(stderr, "Failed to load script\n");
        return 1;
    }
    
    // Execute
    int result = wvmExecute(state, chunk);
    if (result != 0) {
        fprintf(stderr, "Script error: %s\n", state->errorMessage);
    }
    
    // Call script function from host
    WVMValue args[] = { wvmNumber(42.0) };
    WVMValue ret = wvmCallFunction(state, "scriptFunction", args, 1);
    
    // Cleanup
    wvmChunkDestroy(chunk);
    wvmStateDestroy(state);
    
    return 0;
}
```

## Example 6: Custom Target

### Goal

Add support for a custom RISC-V-like architecture.

### Machine Description (custom_risc.json)

```json
{
  "name": "custom_risc",
  "vendor": "Custom",
  "endianness": "little",
  "pointerSize": 4,
  "registers": [
    {"id": 0, "name": "x0", "class": "gpr", "allocatable": false},
    {"id": 1, "name": "x1", "class": "gpr", "allocatable": true},
    {"id": 2, "name": "x2", "class": "gpr", "allocatable": true},
    {"id": 31, "name": "x31", "class": "gpr", "allocatable": true}
  ],
  "instructions": [
    {
      "id": 0,
      "mnemonic": "add",
      "operands": [
        {"name": "rd", "type": "reg", "class": "gpr", "isOutput": true},
        {"name": "rs1", "type": "reg", "class": "gpr", "isInput": true},
        {"name": "rs2", "type": "reg", "class": "gpr", "isInput": true}
      ],
      "encoding": {
        "format": "r_type",
        "opcode": 51,
        "funct3": 0,
        "funct7": 0
      }
    }
  ]
}
```

### Usage

```c
// Load custom machine
MDSMachine *machine = mdsLoadMachineJSON("custom_risc.json");

// Validate
if (!mdsValidateMachine(machine)) {
    fprintf(stderr, "Invalid machine description\n");
    return 1;
}

// Use in compilation
MDSProgram *program = mdsSelectInstructions(malModule, machine);
```

## Example 7: Optimization Pass

### Goal

Implement a custom optimization pass.

### Dead Store Elimination

```c
void eliminateDeadStores(MALFunction *func) {
    // Compute live variables
    BitSet *liveOut = computeLiveOut(func);
    
    // Iterate backwards through instructions
    for (int i = func->instrCount - 1; i >= 0; i--) {
        MALInst *inst = &func->instructions[i];
        
        // If this is a store and the destination is not live, remove it
        if (inst->opcode == MAL_OP_STORE) {
            MALReg dst = inst->dst;
            
            if (!bitSetContains(liveOut[i], dst)) {
                // Dead store - remove it
                malRemoveInstruction(func, i);
            }
        }
    }
}
```

## Best Practices from Examples

1. **Validate Early**: Check IR validity after construction
2. **Optimize Incrementally**: Apply optimizations in stages
3. **Profile Guided**: Use runtime information for better optimization
4. **Modular Design**: Separate parsing, compilation, and optimization
5. **Error Handling**: Check return values and handle errors gracefully
6. **Resource Management**: Clean up resources in reverse order of creation
7. **Test Thoroughly**: Verify correctness with diverse inputs

## Conclusion

These examples demonstrate WIRBLE's versatility across different use cases, from simple expression compilers to sophisticated JIT systems. The modular design allows you to use only the components you need while maintaining the flexibility to extend and customize the system.
