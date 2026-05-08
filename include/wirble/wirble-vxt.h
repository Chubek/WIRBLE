/* NOTE
 * This file is CONCEPTUAL and is meant as a GUIDE.
 * You should ENTIRELY rewrite it.
 * But you SHOULD be influenced by it.
 * THIS IS NOT THE FINAL FILE.
 * This is just something to help you find your footing at.
 * */




#ifndef WIRBLE_VXT_H
#define WIRBLE_VXT_H

/*
 * WIRBLE VXT
 * ----------
 * Minimal vector extension facilities for WIRBLE IR.
 *
 * VXT is declarative and machine-driven.
 *
 * Machine definitions embed vector extension information:
 *
 * vector_extensions:
 *   avx2:
 *     vector_bits: 256
 *     gather: true
 *
 * This header intentionally exposes only:
 *   - semantic vector types
 *   - extension metadata
 *   - query APIs
 *   - validation APIs
 *   - lowering interfaces
 *
 * Backend details remain internal.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
|--------------------------------------------------------------------------
| Constants
|--------------------------------------------------------------------------
*/

#define WIRBLE_VXT_NAME_MAX            64
#define WIRBLE_VXT_EXTENSION_MAX       64
#define WIRBLE_VXT_INSTRUCTION_MAX     256
#define WIRBLE_VXT_REGISTER_MAX        256

/*
|--------------------------------------------------------------------------
| Vector Kinds
|--------------------------------------------------------------------------
*/

typedef enum wirble_vxt_vector_kind {
    WIRBLE_VXT_FIXED = 0,
    WIRBLE_VXT_SCALABLE,
    WIRBLE_VXT_PREDICATE
} wirble_vxt_vector_kind_t;

/*
|--------------------------------------------------------------------------
| Semantic Vector Types
|--------------------------------------------------------------------------
*/

typedef struct wirble_vxt_vector_type {
    wirble_vxt_vector_kind_t kind;

    uint32_t lanes;
    uint32_t bits;

    bool scalable;
} wirble_vxt_vector_type_t;

/*
|--------------------------------------------------------------------------
| Semantic Instruction Description
|--------------------------------------------------------------------------
|
| Represents semantic operations instead of raw ISA mnemonics.
|
| Example:
|
| semantic = "add"
| element_type = "i32"
| lanes = 8
|
*/

typedef struct wirble_vxt_instruction_semantic {
    char semantic[WIRBLE_VXT_NAME_MAX];

    char element_type[16];

    uint32_t lanes;
    uint32_t vector_bits;
} wirble_vxt_instruction_semantic_t;

/*
|--------------------------------------------------------------------------
| Vector Extension Definition
|--------------------------------------------------------------------------
|
| Example:
|
| avx2:
|   vector_bits: 256
|   gather: true
|
*/

typedef struct wirble_vxt_extension {
    char name[WIRBLE_VXT_NAME_MAX];

    uint32_t vector_bits;

    bool masks;
    bool gather;
    bool scatter;
    bool predication;
    bool scalable;

    uint32_t instruction_count;

    char instructions
        [WIRBLE_VXT_INSTRUCTION_MAX]
        [WIRBLE_VXT_NAME_MAX];

} wirble_vxt_extension_t;

/*
|--------------------------------------------------------------------------
| Machine Vector Extension State
|--------------------------------------------------------------------------
*/

typedef struct wirble_vxt_machine {
    uint32_t extension_count;

    wirble_vxt_extension_t
        extensions[WIRBLE_VXT_EXTENSION_MAX];

} wirble_vxt_machine_t;

/*
|--------------------------------------------------------------------------
| Validation Errors
|--------------------------------------------------------------------------
*/

typedef struct wirble_vxt_error {
    char message[256];

    char object_name[WIRBLE_VXT_NAME_MAX];
} wirble_vxt_error_t;

/*
|--------------------------------------------------------------------------
| Parser APIs
|--------------------------------------------------------------------------
|
| Load vector extension definitions from:
|   - YAML
|   - JSON
|   - XML
|
*/

bool wirble_vxt_load_yaml(
    wirble_vxt_machine_t* machine,
    const char* path
);

bool wirble_vxt_load_json(
    wirble_vxt_machine_t* machine,
    const char* path
);

bool wirble_vxt_load_xml(
    wirble_vxt_machine_t* machine,
    const char* path
);

/*
|--------------------------------------------------------------------------
| Validation APIs
|--------------------------------------------------------------------------
*/

bool wirble_vxt_validate(
    const wirble_vxt_machine_t* machine,
    wirble_vxt_error_t* error
);

/*
|--------------------------------------------------------------------------
| Extension Queries
|--------------------------------------------------------------------------
*/

const wirble_vxt_extension_t*
wirble_vxt_get_extension(
    const wirble_vxt_machine_t* machine,
    const char* name
);

bool wirble_vxt_has_extension(
    const wirble_vxt_machine_t* machine,
    const char* name
);

/*
|--------------------------------------------------------------------------
| Capability Queries
|--------------------------------------------------------------------------
*/

bool wirble_vxt_supports_masks(
    const wirble_vxt_machine_t* machine
);

bool wirble_vxt_supports_gather(
    const wirble_vxt_machine_t* machine
);

bool wirble_vxt_supports_scatter(
    const wirble_vxt_machine_t* machine
);

bool wirble_vxt_supports_predication(
    const wirble_vxt_machine_t* machine
);

bool wirble_vxt_supports_scalable_vectors(
    const wirble_vxt_machine_t* machine
);

/*
|--------------------------------------------------------------------------
| Instruction Queries
|--------------------------------------------------------------------------
*/

bool wirble_vxt_has_instruction(
    const wirble_vxt_machine_t* machine,
    const char* instruction
);

/*
|--------------------------------------------------------------------------
| Vector Type Helpers
|--------------------------------------------------------------------------
*/

uint32_t wirble_vxt_vector_bits(
    const wirble_vxt_vector_type_t* type
);

uint32_t wirble_vxt_vector_lanes(
    const wirble_vxt_vector_type_t* type
);

bool wirble_vxt_is_scalable(
    const wirble_vxt_vector_type_t* type
);

bool wirble_vxt_is_predicate(
    const wirble_vxt_vector_type_t* type
);

/*
|--------------------------------------------------------------------------
| Lowering Interface
|--------------------------------------------------------------------------
|
| Lowers semantic vector operations into target operations.
|
| Example:
|
| semantic "add"
| ↓
| vpaddd
|
*/

typedef struct wirble_vxt_lower_result {
    bool success;

    char instruction[WIRBLE_VXT_NAME_MAX];
} wirble_vxt_lower_result_t;

bool wirble_vxt_lower_operation(
    const wirble_vxt_machine_t* machine,

    const wirble_vxt_instruction_semantic_t* semantic,

    wirble_vxt_lower_result_t* result
);

/*
|--------------------------------------------------------------------------
| Canonicalization / Rewrite APIs
|--------------------------------------------------------------------------
*/

bool wirble_vxt_canonicalize(
    void* ir_node
);

bool wirble_vxt_optimize_patterns(
    void* ir_module
);

/*
|--------------------------------------------------------------------------
| Debug APIs
|--------------------------------------------------------------------------
*/

void wirble_vxt_dump_machine(
    const wirble_vxt_machine_t* machine
);

void wirble_vxt_dump_extension(
    const wirble_vxt_extension_t* extension
);

#ifdef __cplusplus
}
#endif

#endif
