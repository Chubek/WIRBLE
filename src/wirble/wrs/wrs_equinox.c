#include "wrs_internal.h"

#include <equinox/egraph.h>
#include <equinox/rewrite.h>

static uint32_t op_of(WILNodeKind k){ return (uint32_t)k + 1u; }

static eqx_eclass_id_t add_node(eqx_egraph_t* g, WILNode* n){
  eqx_eclass_id_t ids[4];
  WILIndex i,c;
  if(!n) return eqx_egraph_add(g,0u,0u,NULL);
  c = wil_node_input_count(n);
  for(i=0;i<c && i<4u;++i) ids[i]=add_node(g,wil_node_input(n,i));
  return eqx_egraph_add(g,op_of(wil_node_kind(n)),(size_t)c, c?ids:NULL);
}

int wrs_nodes_equivalent(WILNode* a, WILNode* b){
  eqx_egraph_t* g;
  eqx_egraph_config_t cfg = eqx_egraph_config_default();
  eqx_eclass_id_t ia,ib;
  if(a==b) return 1;
  if(!a||!b) return 0;
  cfg.max_iterations=4u;
  g = eqx_egraph_create(&cfg);
  if(!g) return 0;
  ia=add_node(g,a); ib=add_node(g,b);
  eqx_egraph_rebuild(g);
  /* lightweight saturation for commutative ops only */
  if((wil_node_kind(a)==WIL_NODE_ADD && wil_node_kind(b)==WIL_NODE_ADD) || (wil_node_kind(a)==WIL_NODE_MUL && wil_node_kind(b)==WIL_NODE_MUL)){
    eqx_pattern_t* x=eqx_pattern_var("x"); eqx_pattern_t* y=eqx_pattern_var("y");
    eqx_pattern_t* lch[2]={x,y}; eqx_pattern_t* rch[2]={y,x};
    eqx_pattern_t* l=eqx_pattern_app(op_of(wil_node_kind(a)),lch,2u);
    eqx_pattern_t* r=eqx_pattern_app(op_of(wil_node_kind(a)),rch,2u);
    eqx_rewrite_rule_t* rr=eqx_rewrite_rule_create("comm",l,r,NULL);
    eqx_rewrite_apply_all(rr,g); eqx_egraph_rebuild(g);
    eqx_rewrite_rule_destroy(rr);
  }
  { int eq=eqx_egraph_equiv(g,ia,ib)?1:0; eqx_egraph_destroy(g); return eq; }
}
