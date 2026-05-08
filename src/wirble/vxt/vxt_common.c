#include <wirble/wirble-vxt.h>

#include <stdio.h>
#include <string.h>

static void vxt_clear_error(wirble_vxt_error_t* error){ if(error){ error->message[0]='\0'; error->object_name[0]='\0'; }}
static void vxt_set_error(wirble_vxt_error_t* error,const char* msg,const char* name){ if(!error) return; snprintf(error->message,sizeof(error->message),"%s",msg?msg:""); snprintf(error->object_name,sizeof(error->object_name),"%s",name?name:""); }

bool wirble_vxt_validate(const wirble_vxt_machine_t* machine, wirble_vxt_error_t* error){ uint32_t i,j; vxt_clear_error(error); if(!machine){ vxt_set_error(error,"null machine",""); return false; } if(machine->extension_count>WIRBLE_VXT_EXTENSION_MAX){ vxt_set_error(error,"extension_count overflow",""); return false; } for(i=0;i<machine->extension_count;++i){ const wirble_vxt_extension_t* e=&machine->extensions[i]; if(e->name[0]=='\0'){ vxt_set_error(error,"empty extension name",e->name); return false; } if(e->instruction_count>WIRBLE_VXT_INSTRUCTION_MAX){ vxt_set_error(error,"instruction_count overflow",e->name); return false; } for(j=0;j<e->instruction_count;++j){ if(e->instructions[j][0]=='\0'){ vxt_set_error(error,"empty instruction entry",e->name); return false; } } } return true; }

const wirble_vxt_extension_t* wirble_vxt_get_extension(const wirble_vxt_machine_t* machine,const char* name){ uint32_t i; if(!machine||!name) return NULL; for(i=0;i<machine->extension_count;++i){ if(strcmp(machine->extensions[i].name,name)==0) return &machine->extensions[i]; } return NULL; }
bool wirble_vxt_has_extension(const wirble_vxt_machine_t* machine,const char* name){ return wirble_vxt_get_extension(machine,name)!=NULL; }

static bool vxt_any_cap(const wirble_vxt_machine_t* machine,int cap){ uint32_t i; if(!machine) return false; for(i=0;i<machine->extension_count;++i){ const wirble_vxt_extension_t* e=&machine->extensions[i]; if((cap==0&&e->masks)||(cap==1&&e->gather)||(cap==2&&e->scatter)||(cap==3&&e->predication)||(cap==4&&e->scalable)) return true; } return false; }
bool wirble_vxt_supports_masks(const wirble_vxt_machine_t* m){ return vxt_any_cap(m,0); }
bool wirble_vxt_supports_gather(const wirble_vxt_machine_t* m){ return vxt_any_cap(m,1); }
bool wirble_vxt_supports_scatter(const wirble_vxt_machine_t* m){ return vxt_any_cap(m,2); }
bool wirble_vxt_supports_predication(const wirble_vxt_machine_t* m){ return vxt_any_cap(m,3); }
bool wirble_vxt_supports_scalable_vectors(const wirble_vxt_machine_t* m){ return vxt_any_cap(m,4); }

bool wirble_vxt_has_instruction(const wirble_vxt_machine_t* machine,const char* instruction){ uint32_t i,j; if(!machine||!instruction) return false; for(i=0;i<machine->extension_count;++i){ for(j=0;j<machine->extensions[i].instruction_count;++j){ if(strcmp(machine->extensions[i].instructions[j],instruction)==0) return true; }} return false; }

uint32_t wirble_vxt_vector_bits(const wirble_vxt_vector_type_t* type){ return type?type->bits:0u; }
uint32_t wirble_vxt_vector_lanes(const wirble_vxt_vector_type_t* type){ return type?type->lanes:0u; }
bool wirble_vxt_is_scalable(const wirble_vxt_vector_type_t* type){ return type?type->scalable:false; }
bool wirble_vxt_is_predicate(const wirble_vxt_vector_type_t* type){ return type?type->kind==WIRBLE_VXT_PREDICATE:false; }

bool wirble_vxt_load_yaml(wirble_vxt_machine_t* machine,const char* path){ (void)path; if(!machine) return false; memset(machine,0,sizeof(*machine)); return true; }
bool wirble_vxt_load_json(wirble_vxt_machine_t* machine,const char* path){ return wirble_vxt_load_yaml(machine,path); }
bool wirble_vxt_load_xml(wirble_vxt_machine_t* machine,const char* path){ return wirble_vxt_load_yaml(machine,path); }

void wirble_vxt_dump_extension(const wirble_vxt_extension_t* ext){ uint32_t i; if(!ext) return; printf("vxt-extension %s bits=%u\n",ext->name,ext->vector_bits); for(i=0;i<ext->instruction_count;++i) printf("  %s\n",ext->instructions[i]); }
void wirble_vxt_dump_machine(const wirble_vxt_machine_t* machine){ uint32_t i; if(!machine) return; for(i=0;i<machine->extension_count;++i) wirble_vxt_dump_extension(&machine->extensions[i]); }

bool wirble_vxt_canonicalize(void* ir_node){ (void)ir_node; return true; }
bool wirble_vxt_optimize_patterns(void* ir_module){ (void)ir_module; return true; }
