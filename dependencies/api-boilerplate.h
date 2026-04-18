/**
 * @file api-boilerplate.h
 * @brief Portable API macros for the APIs, built on top of hedley.h
 */

#ifndef API_BOILERPLATE_H
#define API_BOILERPLATE_H

#include "hedley.h"

/* =========================================================================
 * VERSION
 * ========================================================================= */

#define WIRBLE_VERSION_MAJOR 1
#define WIRBLE_VERSION_MINOR 0
#define WIRBLE_VERSION_PATCH 0

/**
 * Numeric version: encoded as (major * 10000) + (minor * 100) + patch
 * e.g. 1.2.3  ->  10203
 *      2.14.7 ->  21407
 */
#define WIRBLE_VERSION_NUM                                                    \
  ((WIRBLE_VERSION_MAJOR * 10000) + (WIRBLE_VERSION_MINOR * 100)              \
   + (WIRBLE_VERSION_PATCH))

/** String version, e.g. "1.0.0" */
#define WIRBLE_STRINGIFY_(x) #x
#define WIRBLE_STRINGIFY(x) WIRBLE_STRINGIFY_ (x)

#define WIRBLE_VERSION_STR                                                    \
  WIRBLE_STRINGIFY (WIRBLE_VERSION_MAJOR)                                     \
  "." WIRBLE_STRINGIFY (WIRBLE_VERSION_MINOR) "." WIRBLE_STRINGIFY (          \
      WIRBLE_VERSION_PATCH)

/** Human-readable "about" string */
#define WIRBLE_ABOUT                                                          \
  "WIRBLE Library v" WIRBLE_VERSION_STR                                       \
  " -- Copyright (C) 2026 WIRBLE Authors. All rights reserved."

/* =========================================================================
 * SYMBOL VISIBILITY  (hedley.h lines 1641-1676)
 *
 *  Define WIRBLE_BUILD when compiling the library itself.
 *  Consumers only include this header; they must NOT define WIRBLE_BUILD.
 * ========================================================================= */

/** Public API symbol – exported when building, imported when consuming. */
#if defined(WIRBLE_BUILD)
#define API_BOILERPLATE                                                       \
  HEDLEY_PUBLIC /* __declspec(dllexport) / visibility("default") */
#else
#define API_BOILERPLATE                                                       \
  HEDLEY_IMPORT /* __declspec(dllimport) / extern               */
#endif

/**
 * Private symbol – hidden from the shared-library ABI.
 * Use on internal helper functions that must never be called by consumers.
 * (hedley.h: HEDLEY_PRIVATE -> visibility("hidden") on ELF, empty on MSVC)
 */
#define WIRBLE_PRIVATE HEDLEY_PRIVATE

/**
 * For functions that are provided only in a shared library and MUST be
 * loaded at runtime via dlopen/LoadLibrary + dlsym/GetProcAddress.
 * Marks the declaration as an import so that no static-link stub is emitted.
 */
#define WIRBLE_DYNLIB HEDLEY_IMPORT

/* =========================================================================
 * INLINING  (hedley.h lines 1555-1638)
 * ========================================================================= */

/** Force inlining: __attribute__((always_inline)) / __forceinline */
#define WIRBLE_ALWAYS_INLINE HEDLEY_ALWAYS_INLINE

/** Prevent inlining: __attribute__((noinline)) / __declspec(noinline) */
#define WIRBLE_NEVER_INLINE HEDLEY_NEVER_INLINE

/* =========================================================================
 * FUNCTION ATTRIBUTES  (hedley.h lines 1399-1497)
 * ========================================================================= */

/**
 * Pure function: return value depends only on arguments + global state;
 * no observable side effects.  Enables CSE and other optimisations.
 * (hedley.h: __attribute__((pure)))
 */
#define WIRBLE_PURE HEDLEY_PURE

/**
 * Const function: return value depends ONLY on arguments (no globals read);
 * strictly stronger than PURE.
 * (hedley.h: __attribute__((const)) or alias to HEDLEY_PURE)
 */
#define WIRBLE_CONST HEDLEY_CONST

/**
 * Malloc-like: returned pointer does not alias any existing object.
 * (hedley.h: __attribute__((malloc)) / __declspec(restrict))
 *
 * Usage:
 *   API_BOILERPLATE WIRBLE_MALLOC(size)
 *   void *wirble_alloc(size_t size);
 *
 * The parameter name 'n' is kept as a documentation convention; it is
 * not evaluated – only the annotation matters to the compiler.
 */
#define WIRBLE_MALLOC(n) HEDLEY_MALLOC /* n = size (documentation only) */

/* =========================================================================
 * BRANCH PREDICTION HINTS  (hedley.h lines 1336-1393)
 * ========================================================================= */

/** Hint that the expression is TRUE in the common case. */
#define WIRBLE_LIKELY(expr) HEDLEY_LIKELY (expr)

/** Hint that the expression is FALSE in the common case. */
#define WIRBLE_UNLIKELY(expr) HEDLEY_UNLIKELY (expr)

/* =========================================================================
 * HOT / COLD  (not in this hedley.h build; defined here in Hedley style)
 * ========================================================================= */

#if !defined(WIRBLE_HOT)
#if HEDLEY_HAS_ATTRIBUTE(hot) || HEDLEY_GCC_VERSION_CHECK(4, 3, 0)
#define WIRBLE_HOT __attribute__ ((__hot__))
#else
#define WIRBLE_HOT
#endif
#endif

#if !defined(WIRBLE_COLD)
#if HEDLEY_HAS_ATTRIBUTE(cold) || HEDLEY_GCC_VERSION_CHECK(4, 3, 0)
#define WIRBLE_COLD __attribute__ ((__cold__))
#else
#define WIRBLE_COLD
#endif
#endif

/* =========================================================================
 * PRINTF FORMAT CHECKING  (hedley.h lines 1289-1318)
 *
 * Usage:
 *   API_BOILERPLATE WIRBLE_PRINTF(1, 2)
 *   void wirble_log(const char *fmt, ...);
 *
 *   fmt_idx    – 1-based index of the format-string argument
 *   first_arg  – 1-based index of the first variadic argument
 *                (use 0 for vprintf-style functions)
 * ========================================================================= */
#define WIRBLE_PRINTF(fmt_idx, first_arg)                                     \
  HEDLEY_PRINTF_FORMAT (fmt_idx, first_arg)

/* =========================================================================
 * NO-RETURN  (hedley.h lines 1145-1186)
 *
 * Mark functions that never return (abort, longjmp wrappers, etc.)
 * Compiler uses this to suppress "missing return" warnings on callers.
 * ========================================================================= */
#define WIRBLE_NORETURN HEDLEY_NO_RETURN

/* =========================================================================
 * PLUGIN METADATA MACROS
 *
 * These are purely declarative metadata tags embedded inside plugin
 * translation units.  They follow the same convention used by projects
 * such as GStreamer, LADSPA, LV2, and Audacity:
 *
 *   • Each macro expands to a static const char[] so the information
 *     survives in the object file and can be queried with `strings` or
 *     a dedicated loader.
 *   • WIRBLE_PLUGIN_AUTHOR / WIRBLE_PLUGIN_MAINTAINER may appear more
 *     than once in the same translation unit.
 *   • WIRBLE_PLUGIN_CALL_ON_INIT / WIRBLE_PLUGIN_CALL_ON_EXIT register
 *     constructor/destructor callbacks using __attribute__((constructor))
 *     on GCC/Clang or #pragma init_seg on MSVC.
 * ========================================================================= */

/** Plugin canonical name, e.g. WIRBLE_PLUGIN_NAME("my-effect") */
#define WIRBLE_PLUGIN_NAME(n)                                                 \
  static const char                                                           \
      _wirble_plugin_name[] HEDLEY_DIAGNOSTIC_DISABLE_CPP98_COMPAT_WRAP_ (    \
          (unused))                                                           \
      = "" n ""

/** Plugin version string, e.g. WIRBLE_PLUGIN_VERSION("1.2.3") */
#define WIRBLE_PLUGIN_VERSION(v)                                              \
  static const char                                                           \
      _wirble_plugin_version[] HEDLEY_DIAGNOSTIC_DISABLE_CPP98_COMPAT_WRAP_ ( \
          (unused))                                                           \
      = "" v ""

/** SPDX license identifier, e.g. WIRBLE_PLUGIN_LICENSE("MIT") */
#define WIRBLE_PLUGIN_LICENSE(l)                                              \
  static const char                                                           \
      _wirble_plugin_license[] HEDLEY_DIAGNOSTIC_DISABLE_CPP98_COMPAT_WRAP_ ( \
          (unused))                                                           \
      = "" l ""

/** Short description, e.g. WIRBLE_PLUGIN_DESC("Adds reverb to audio") */
#define WIRBLE_PLUGIN_DESC(d)                                                 \
  static const char                                                           \
      _wirble_plugin_desc[] HEDLEY_DIAGNOSTIC_DISABLE_CPP98_COMPAT_WRAP_ (    \
          (unused))                                                           \
      = "" d ""

/** Homepage / repository URL, e.g.
 * WIRBLE_PLUGIN_HOMEPAGE("https://example.com") */
#define WIRBLE_PLUGIN_HOMEPAGE(u)                                             \
  static const char _wirble_plugin_homepage                                   \
      [] HEDLEY_DIAGNOSTIC_DISABLE_CPP98_COMPAT_WRAP_ ((unused))              \
      = "" u ""

/*
 * For MAINTAINER and AUTHOR we use a __COUNTER__-based unique symbol so the
 * macro can be repeated multiple times in the same translation unit without
 * triggering duplicate-symbol errors.
 *
 * __COUNTER__ is supported by GCC >= 4.3, Clang, MSVC >= 2005, and ICC.
 * On compilers without it we fall back to __LINE__ (still allows multiple
 * authors as long as they are on different lines, which is the normal case).
 */
#if defined(__COUNTER__)
#define WIRBLE_PLUGIN_UNIQUE_ __COUNTER__
#else
#define WIRBLE_PLUGIN_UNIQUE_ __LINE__
#endif

/* Internal paste helpers */
#define WIRBLE_PASTE__(a, b) a##b
#define WIRBLE_PASTE_(a, b) WIRBLE_PASTE__ (a, b)

/**
 * Maintainer record.  May appear once or more.
 *   WIRBLE_PLUGIN_MAINTAINER("Alice Smith", "alice@example.com")
 */
#define WIRBLE_PLUGIN_MAINTAINER(n, e)                                        \
  static const char WIRBLE_PASTE_ (_wirble_plugin_maintainer_,                \
                                   WIRBLE_PLUGIN_UNIQUE_)                     \
      [] HEDLEY_DIAGNOSTIC_DISABLE_CPP98_COMPAT_WRAP_ ((unused))              \
      = "maintainer:" n " <" e ">"

/**
 * Author record.  May appear multiple times.
 *   WIRBLE_PLUGIN_AUTHOR("Bob Jones", "bob@example.com")
 *   WIRBLE_PLUGIN_AUTHOR("Carol Wu",  "carol@example.com")
 */
#define WIRBLE_PLUGIN_AUTHOR(a, e)                                            \
  static const char WIRBLE_PASTE_ (_wirble_plugin_author_,                    \
                                   WIRBLE_PLUGIN_UNIQUE_)                     \
      [] HEDLEY_DIAGNOSTIC_DISABLE_CPP98_COMPAT_WRAP_ ((unused))              \
      = "author:" a " <" e ">"

/* -------------------------------------------------------------------------
 * Constructor / destructor callbacks
 * ------------------------------------------------------------------------- */

#if defined(_MSC_VER)
/*
 * MSVC: use CRT init-seg hooks.
 * The function is placed in the .CRT$XCU (constructor) or .CRT$XPU
 * (pre-termination) segment and called automatically by the CRT.
 */
#pragma section(".CRT$XCU", read)
#pragma section(".CRT$XPU", read)

#define WIRBLE_PLUGIN_CALL_ON_INIT(fn)                                        \
  static void fn (void);                                                      \
  __declspec (allocate (".CRT$XCU")) static void (                            \
      *WIRBLE_PASTE_ (_wirble_init_ptr_, __COUNTER__)) (void)                 \
      = (fn);                                                                 \
  static void fn (void)

#define WIRBLE_PLUGIN_CALL_ON_EXIT(fn)                                        \
  static void fn (void);                                                      \
  __declspec (allocate (".CRT$XPU")) static void (                            \
      *WIRBLE_PASTE_ (_wirble_exit_ptr_, __COUNTER__)) (void)                 \
      = (fn);                                                                 \
  static void fn (void)

#elif defined(__GNUC__) || defined(__clang__)
/*
 * GCC / Clang: __attribute__((constructor)) / __attribute__((destructor))
 * Priority 101 leaves room below 100 for C++ static objects.
 */
#define WIRBLE_PLUGIN_CALL_ON_INIT(fn)                                        \
  __attribute__ ((__constructor__ (101))) static void fn (void)

#define WIRBLE_PLUGIN_CALL_ON_EXIT(fn)                                        \
  __attribute__ ((__destructor__ (101))) static void fn (void)

#else
/* Unknown toolchain: best-effort via _init / _fini weak symbols. */
#define WIRBLE_PLUGIN_CALL_ON_INIT(fn) static void fn (void)
#define WIRBLE_PLUGIN_CALL_ON_EXIT(fn) static void fn (void)
#warning "WIRBLE: automatic plugin init/exit not supported on this compiler."
#endif

/*=====================================================================
 * For blocks of code in C++
 *---------------------------------------------------------------------*/

#ifdef __cplusplus
#define WIRBLE_DECL extern "C"
#define WIRBLE_BEGIN_DECLS                                                    \
  extern "C"                                                                  \
  {
#define WIRBLE_END_DECLS }
#else
#define WIRBLE_DECL
#define WIRBLE_BEGIN_DECLS
#define WIRBLE_END_DECLS
#endif

#endif /* API_BOILERPLATE_H */
