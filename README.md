# WIRBLE: A Minimal C Library for Code Generation

WIRBLE is a minimal C library for code generation. With WIRBLE, you can use one of the interfaces to write code in WIL or WIRBLE Intermediate Language, and compile it into one of the targets which is defiend via WIRBLE's extensibility layer. WIRBLE ships with three targets: x86-64, Aarch64 and WASM, all three defined via the target API (`include/wirble-target.h`). The targets are written using the API specified in this header file, compiled to a shared library, and the WIRBLE Compiler will load the shared library and use the exposed functions to compile the WIL.

The WIL itself is defined via a series of functions exposed in `include/wirble-wil.h`. These functions define an IR in the Sea of Nodes style. Here's an example of WIL code:

```


```
