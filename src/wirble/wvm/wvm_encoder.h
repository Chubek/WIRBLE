#ifndef WIRBLE_WVM_ENCODER_H
#define WIRBLE_WVM_ENCODER_H

#include <wirble/wirble-tos.h>
#include <wirble/wirble-wvm.h>

char *wvm_strdup (const char *text);
WVMPrototype *wvmPrototypeFromTOS (const TOSProgram *program,
                                   const char *name,
                                   const char *source);
void wvmPrototypeDestroyInternal (WVMState *vm, WVMPrototype *proto);
int wvmSerializePrototype (const WVMPrototype *proto, WVMBuffer *buf);
WVMPrototype *wvmDeserializePrototype (WVMState *vm, WVMReader *reader);
void wvmSetErrorf (WVMState *vm, const char *message);

#endif /* WIRBLE_WVM_ENCODER_H */
