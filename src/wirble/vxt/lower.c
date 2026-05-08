#include <wirble/wirble-vxt.h>

#include <string.h>

bool wirble_vxt_lower_operation(const wirble_vxt_machine_t* machine,const wirble_vxt_instruction_semantic_t* semantic,wirble_vxt_lower_result_t* result){ uint32_t i,j; if(!machine||!semantic||!result) return false; result->success=false; result->instruction[0]='\0'; for(i=0;i<machine->extension_count;++i){ const wirble_vxt_extension_t* ext=&machine->extensions[i]; if(ext->vector_bits!=0u && semantic->vector_bits!=0u && ext->vector_bits<semantic->vector_bits) continue; for(j=0;j<ext->instruction_count;++j){ if(strstr(ext->instructions[j],semantic->semantic)!=NULL){ strncpy(result->instruction,ext->instructions[j],WIRBLE_VXT_NAME_MAX-1u); result->instruction[WIRBLE_VXT_NAME_MAX-1u]='\0'; result->success=true; return true; } } } return false; }
