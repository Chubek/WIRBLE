#ifndef EQUINOX_H
#define EQUINOX_H

#ifdef __cplusplus
extern "C" {
#endif

/* Main header file for Equinox e-graph library */

#include "egraph.h"
#include "eclass.h"
#include "enode.h"
#include "unionfind.h"
#include "hashcons.h"
#include "rewrite.h"

/* Version information */
#define EQUINOX_VERSION_MAJOR 0
#define EQUINOX_VERSION_MINOR 1
#define EQUINOX_VERSION_PATCH 0

/* Library initialization and cleanup */
int equinox_init(void);
void equinox_cleanup(void);

/* Get version string */
const char* equinox_version(void);

#ifdef __cplusplus
}
#endif

#endif /* EQUINOX_H */
