#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([TimePlugin], [0.1], [jackoalan@gmail.com])
AC_CONFIG_SRCDIR([src/Server.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([TimePlugin], [0.1])

# Libtool Stuff
AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)

# Plugin Dirs
m4_include([../plugininclude.m4])
AC_SUBST([webinstalldir], ['${webplugindir}/TimePlugin'])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h sys/time.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_FUNCS([gettimeofday])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
