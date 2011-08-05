#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([AnimationPlugin], [0.1], [jackoalan@gmail.com])
AC_CONFIG_SRCDIR([src/Server.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([AnimationPlugin], [0.1])

# Libtool Stuff
LT_INIT([])
AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)

# Plugin Dirs
m4_include([../plugininclude.m4])
AC_SUBST([webinstalldir], ['${webplugindir}/AnimationPlugin'])


# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([string.h sys/time.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_FUNCS([gettimeofday strcasecmp])

AC_CONFIG_FILES([Makefile src/Makefile web/Makefile])
AC_OUTPUT
