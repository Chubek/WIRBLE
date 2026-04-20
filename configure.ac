AC_INIT([WIRBLE], [0.4], [bugs@example.invalid])
AC_CONFIG_SRCDIR([README.md])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIRS([m4])
AM_INIT_AUTOMAKE([foreign subdir-objects])

AC_PROG_INSTALL
AC_PROG_MKDIR_P

AC_ARG_ENABLE(
  [install-docs],
  [AS_HELP_STRING([--enable-install-docs], [generate and install Doxygen documentation])],
  [enable_install_docs=$enableval],
  [enable_install_docs=no]
)
AC_ARG_ENABLE(
  [install-manfiles],
  [AS_HELP_STRING([--enable-install-manfiles], [generate and install Doxygen manpages])],
  [enable_install_manfiles=$enableval],
  [enable_install_manfiles=no]
)
AC_ARG_ENABLE(
  [install-manifest],
  [AS_HELP_STRING([--enable-install-manifest], [install manifests under $(datadir)/wirble/manifests])],
  [enable_install_manifest=$enableval],
  [enable_install_manifest=no]
)
AC_ARG_ENABLE(
  [install-toolchain],
  [AS_HELP_STRING([--disable-install-toolchain], [do not install toolchain binaries])],
  [enable_install_toolchain=$enableval],
  [enable_install_toolchain=yes]
)
AC_ARG_ENABLE(
  [install-machines],
  [AS_HELP_STRING([--disable-install-machines], [do not install machine descriptions under manifests/machines])],
  [enable_install_machines=$enableval],
  [enable_install_machines=yes]
)

AM_CONDITIONAL([INSTALL_DOCS], [test "x$enable_install_docs" = "xyes"])
AM_CONDITIONAL([INSTALL_MANFILES], [test "x$enable_install_manfiles" = "xyes"])
AM_CONDITIONAL([INSTALL_MANIFEST], [test "x$enable_install_manifest" = "xyes"])
AM_CONDITIONAL([INSTALL_TOOLCHAIN], [test "x$enable_install_toolchain" = "xyes"])
AM_CONDITIONAL([INSTALL_MACHINES], [test "x$enable_install_machines" = "xyes"])

AC_CHECK_PROGS([DOXYGEN], [doxygen], [:])
AC_CHECK_PROGS([DOT], [dot], [:])

WIRBLE_PROJECT_VERSION='0.4'
WIRBLE_TOP_SRCDIR='@abs_top_srcdir@'
WIRBLE_OUTPUT_DIRECTORY='@abs_top_builddir@/docs/doxygen'
WIRBLE_STRIP_FROM_INC_PATH='@abs_top_srcdir@/include @abs_top_srcdir@/src'
WIRBLE_LAYOUT_FILE='@abs_top_srcdir@/docs/layout.xml'
WIRBLE_WARN_LOGFILE='@abs_top_builddir@/docs/doxygen-warnings.log'
if test "x$DOT" = "x:"; then
  WIRBLE_HAVE_DOT='NO'
else
  WIRBLE_HAVE_DOT='YES'
fi

AC_SUBST([WIRBLE_PROJECT_VERSION])
AC_SUBST([WIRBLE_TOP_SRCDIR])
AC_SUBST([WIRBLE_OUTPUT_DIRECTORY])
AC_SUBST([WIRBLE_STRIP_FROM_INC_PATH])
AC_SUBST([WIRBLE_LAYOUT_FILE])
AC_SUBST([WIRBLE_WARN_LOGFILE])
AC_SUBST([WIRBLE_HAVE_DOT])

AC_CONFIG_FILES([
  Makefile
  dependencies/Makefile
  stdrewrite/Makefile
  manifests/Makefile
  machines/Makefile
  toolchain/Makefile
  examples/Makefile
  docs/Makefile
  docs/Doxyfile
])
AC_OUTPUT
