#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([VisualiserPlugin], [0.1], [jackoalan@gmail.com])
AC_CONFIG_SRCDIR([src/Server.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([VisualiserPlugin], [0.1])

# Libtool Stuff
AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)

# Plugin Dirs
m4_include([../plugininclude.m4])
AC_SUBST([webinstalldir], ['${webplugindir}/VisualiserPlugin'])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
#AC_SEARCH_LIBS([fftw_plan_dft_r2c_1d], [fftw3], [], [AC_MSG_ERROR([LIBFFTW3 not found. Unable to Make Visualiser plugin])])
AC_SEARCH_LIBS([snd_pcm_open], [asound], [], [AC_MSG_RESULT([ALSA's library not found. Building without ALSA support])])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h string.h unistd.h])
AC_CHECK_HEADERS([fftw3.h], [], [AC_MSG_ERROR([FFTW3's headers not found. Unable to make Visualiser plugin])])
AC_CHECK_HEADERS([alsa/asoundlib.h], [], [AC_MSG_RESULT([ALSA's headers not found. Building without ALSA support])])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT16_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_CHECK_FUNCS([pow sqrt strerror])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
