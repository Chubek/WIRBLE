#ifndef WIRBLE_SYMBOLIC_OUTPUT_H
#define WIRBLE_SYMBOLIC_OUTPUT_H

#include <wirble/wirble-mds.h>

#include "api-boilerplate.h"

WIRBLE_BEGIN_DECLS

typedef enum WirbleAsmFlavor
{
  WIRBLE_ASM_FLAVOR_GAS = 0,
  WIRBLE_ASM_FLAVOR_NASM
} WirbleAsmFlavor;

typedef enum WirbleX86Syntax
{
  WIRBLE_X86_SYNTAX_ATT = 0,
  WIRBLE_X86_SYNTAX_INTEL
} WirbleX86Syntax;

int wirbleEmitSymbolicAssembly (const MDSProgram *program, const char *path,
                                WirbleAsmFlavor flavor,
                                WirbleX86Syntax x86Syntax);

WIRBLE_END_DECLS

#endif
