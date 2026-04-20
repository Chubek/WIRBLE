#ifndef WIRBLE_TOS_STREAM_H
#define WIRBLE_TOS_STREAM_H

#include <wirble/wirble-mal.h>
#include <wirble/wirble-mds.h>
#include <wirble/wirble-tos.h>

int tos_clone_operands (MALOperand **outOperands,
                        const MALOperand *operands,
                        uint32_t operandCount);
TOSProgram *tos_clone_program (const TOSProgram *program);
uint32_t tos_find_block_index (const TOSProgram *program, uint32_t blockId);

#endif /* WIRBLE_TOS_STREAM_H */
