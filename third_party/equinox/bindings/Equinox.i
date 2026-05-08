/* Equinox.i - SWIG Interface for Equinox E-graph Library */

%module Equinox

%{
#include "equinox.h"
#include "egraph.h"
#include "enode.h"
#include "eclass.h"
#include "hashcons.h"
#include "rewrite.h"
%}

/* ========================================================================
   XFeats: Core Configuration
   ======================================================================== */

/* String conversion for all char* returns */
%typemap(out) char* %{
#ifdef SWIGPYTHON
    $result = PyUnicode_FromString($1 ? $1 : "");
#elif defined(SWIGJAVA)
    $result = JCALL1(NewStringUTF, jenv, $1 ? $1 : "");
#elif defined(SWIGCSHARP)
    $result = SWIG_csharp_string_callback($1 ? $1 : "");
#elif defined(SWIGRUBY)
    $result = rb_str_new_cstr($1 ? $1 : "");
#elif defined(SWIGJAVASCRIPT)
    $result = SWIG_FromCharPtr($1 ? $1 : "");
#else
    $result = $1;
#endif
%}

/* size_t safe handling */
%typemap(in) size_t %{
#ifdef SWIGPYTHON
    $1 = (size_t)PyLong_AsSize_t($input);
#elif defined(SWIGJAVA)
    $1 = (size_t)$input;
#elif defined(SWIGCSHARP)
    $1 = (size_t)$input;
#else
    $1 = $input;
#endif
%}

/* ========================================================================
   XFeats: Opaque Pointer Types
   ======================================================================== */

%nodefaultctor egraph_t;
%nodefaultdtor egraph_t;
typedef struct egraph_t egraph_t;

%nodefaultctor enode_t;
%nodefaultdtor enode_t;
typedef struct enode_t enode_t;

%nodefaultctor eclass_t;
%nodefaultdtor eclass_t;
typedef struct eclass_t eclass_t;

%nodefaultctor hashcons_t;
%nodefaultdtor hashcons_t;
typedef struct hashcons_t hashcons_t;

%nodefaultctor rewrite_rule_t;
%nodefaultdtor rewrite_rule_t;
typedef struct rewrite_rule_t rewrite_rule_t;

/* ========================================================================
   XFeats: Type Definitions
   ======================================================================== */

typedef uint32_t eqx_id_t;
typedef uint32_t eqx_symbol_t;

/* Enums */
typedef enum {
    EQX_OK = 0,
    EQX_ERROR = -1,
    EQX_INVALID_ARG = -2,
    EQX_OUT_OF_MEMORY = -3
} eqx_status_t;

/* ========================================================================
   XFeats: Null Check Guards
   ======================================================================== */

%exception egraph_create {
    $action
    if (!result) {
        SWIG_exception(SWIG_RuntimeError, "Failed to create egraph");
    }
}

%exception enode_create {
    $action
    if (!result) {
        SWIG_exception(SWIG_RuntimeError, "Failed to create enode");
    }
}

%exception hashcons_create {
    $action
    if (!result) {
        SWIG_exception(SWIG_RuntimeError, "Failed to create hashcons table");
    }
}

/* ========================================================================
   XFeats: Auto-Free Return (newobject)
   ======================================================================== */

%newobject egraph_create;
%newobject enode_create;
%newobject hashcons_create;
%newobject egraph_to_string;
%newobject enode_to_string;

/* ========================================================================
   XFeats: Symbol Renaming
   ======================================================================== */

%rename(EGraph) egraph_t;
%rename(ENode) enode_t;
%rename(EClass) eclass_t;
%rename(HashCons) hashcons_t;
%rename(RewriteRule) rewrite_rule_t;

/* ========================================================================
   XFeats: Documentation Injection
   ======================================================================== */

%feature("docstring") egraph_t "
E-graph data structure for equivalence reasoning.
Maintains equivalence classes of terms with efficient union-find.
";

%feature("docstring") enode_t "
E-node representing a term in the e-graph.
Contains a symbol and children references.
";

%feature("docstring") hashcons_t "
Hash-consing table for structural sharing of e-nodes.
";

/* ========================================================================
   XFeats: Buffer to Array (for enode children)
   ======================================================================== */

#ifdef SWIGPYTHON
%typemap(in) (const eqx_id_t *children, size_t arity) {
    if (!PyList_Check($input)) {
        PyErr_SetString(PyExc_TypeError, "Expected a list of integers");
        SWIG_fail;
    }
    $2 = PyList_Size($input);
    $1 = (eqx_id_t*)malloc($2 * sizeof(eqx_id_t));
    for (size_t i = 0; i < $2; i++) {
        PyObject *item = PyList_GetItem($input, i);
        $1[i] = (eqx_id_t)PyLong_AsUnsignedLong(item);
    }
}

%typemap(freearg) (const eqx_id_t *children, size_t arity) {
    free($1);
}
#endif

#ifdef SWIGJAVA
%typemap(jni) (const eqx_id_t *children, size_t arity) "jintArray"
%typemap(jtype) (const eqx_id_t *children, size_t arity) "int[]"
%typemap(jstype) (const eqx_id_t *children, size_t arity) "int[]"
%typemap(in) (const eqx_id_t *children, size_t arity) {
    $2 = JCALL1(GetArrayLength, jenv, $input);
    $1 = (eqx_id_t*)JCALL2(GetIntArrayElements, jenv, $input, 0);
}
%typemap(freearg) (const eqx_id_t *children, size_t arity) {
    JCALL3(ReleaseIntArrayElements, jenv, $input, (jint*)$1, JNI_ABORT);
}
#endif

/* ========================================================================
   XFeats: Eager Validation
   ======================================================================== */

%typemap(check) egraph_t* {
    if (!$1) {
        SWIG_exception(SWIG_ValueError, "EGraph pointer cannot be NULL");
    }
}

%typemap(check) enode_t* {
    if (!$1) {
        SWIG_exception(SWIG_ValueError, "ENode pointer cannot be NULL");
    }
}

/* ========================================================================
   Core API Functions
   ======================================================================== */

/* E-graph operations */
egraph_t* egraph_create(size_t initial_capacity);
void egraph_destroy(egraph_t* graph);
eqx_id_t egraph_add(egraph_t* graph, eqx_symbol_t symbol, const eqx_id_t* children, size_t arity);
int egraph_merge(egraph_t* graph, eqx_id_t id1, eqx_id_t id2);
eqx_id_t egraph_find(egraph_t* graph, eqx_id_t id);
int egraph_equiv(egraph_t* graph, eqx_id_t id1, eqx_id_t id2);
size_t egraph_size(egraph_t* graph);
char* egraph_to_string(egraph_t* graph);

/* E-node operations */
enode_t* enode_create(eqx_symbol_t symbol, const eqx_id_t* children, size_t arity);
void enode_destroy(enode_t* node);
eqx_symbol_t enode_symbol(enode_t* node);
size_t enode_arity(enode_t* node);
eqx_id_t enode_child(enode_t* node, size_t index);
char* enode_to_string(enode_t* node);

/* Hash-consing operations */
hashcons_t* hashcons_create(size_t capacity);
void hashcons_destroy(hashcons_t* hc);
eqx_id_t hashcons_insert(hashcons_t* hc, enode_t* node);
enode_t* hashcons_lookup(hashcons_t* hc, eqx_id_t id);

/* Rewrite operations */
rewrite_rule_t* rewrite_rule_create(enode_t* lhs, enode_t* rhs);
void rewrite_rule_destroy(rewrite_rule_t* rule);
int rewrite_apply(egraph_t* graph, rewrite_rule_t* rule);

/* ========================================================================
   XFeats: Extended Methods (Python-specific)
   ======================================================================== */

#ifdef SWIGPYTHON
%extend egraph_t {
    %feature("docstring") "__len__" "Returns the number of e-classes in the e-graph.";
    size_t __len__() {
        return egraph_size($self);
    }
    
    %feature("docstring") "__str__" "String representation of the e-graph.";
    const char* __str__() {
        return egraph_to_string($self);
    }
    
    %feature("docstring") "__repr__" "Detailed representation of the e-graph.";
    const char* __repr__() {
        return egraph_to_string($self);
    }
}

%extend enode_t {
    %feature("docstring") "__len__" "Returns the arity (number of children).";
    size_t __len__() {
        return enode_arity($self);
    }
    
    %feature("docstring") "__getitem__" "Access child by index.";
    eqx_id_t __getitem__(size_t index) {
        if (index >= enode_arity($self)) {
            PyErr_SetString(PyExc_IndexError, "Child index out of range");
            return 0;
        }
        return enode_child($self, index);
    }
    
    %feature("docstring") "__str__" "String representation of the e-node.";
    const char* __str__() {
        return enode_to_string($self);
    }
    
    %feature("docstring") "__hash__" "Hash value for the e-node.";
    long __hash__() {
        return (long)enode_symbol($self);
    }
}
#endif

/* ========================================================================
   Language-Specific Configurations
   ======================================================================== */

#ifdef SWIGJAVA
%pragma(java) jniclasscode=%{
    static {
        try {
            System.loadLibrary("equinox");
        } catch (UnsatisfiedLinkError e) {
            System.err.println("Native library failed to load: " + e);
            System.exit(1);
        }
    }
%}
#endif

#ifdef SWIGCSHARP
%pragma(csharp) imclasscode=%{
    [global::System.Runtime.InteropServices.DllImport("equinox", EntryPoint="CSharp_$module")]
%}
#endif

#ifdef SWIGRUBY
%init %{
    rb_require("equinox");
%}
#endif
