#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.


AC_PREREQ([2.68])
AC_INIT([lsd], [0.1], [jackoalan@gmail.com])
AC_CONFIG_SRCDIR([src/OLAWrapper.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_PREFIX_DEFAULT([/usr])
LT_INIT

AM_INIT_AUTOMAKE([lsd], [0.1])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.
AC_SEARCH_LIBS([lt_dlinit], [ltdl])
AC_SEARCH_LIBS([pow], [m])
AC_SEARCH_LIBS([sqlite3_open], [sqlite3])
AC_SEARCH_LIBS([event_base_new], [event])

# Checks for header files.
AC_CHECK_HEADERS([float.h limits.h stdint.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([floor gettimeofday memset pow strcasecmp strchr])

# Output Files
AC_CONFIG_FILES([Makefile src/Makefile Plugins/Makefile web/Makefile])

# Plugin Configure
m4_include([Plugins/pluginlist.m4])

# Get Plugin Directories for compiler flags
m4_include([Plugins/plugininclude.m4])

# Web directory
AC_SUBST([webdir], ['${datadir}/lsd/www'])

# Do
AC_OUTPUT
