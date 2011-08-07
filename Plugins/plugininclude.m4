# Detect Wii
AC_MSG_CHECKING(for HW_RVL)
AC_TRY_COMPILE(,
[
#ifndef HW_RVL
die horribly
#endif
],
wii=true; AC_MSG_RESULT(yes),
wii=false; AC_MSG_RESULT(no),
)
AM_CONDITIONAL([BUILD_RVL], [test x$wii = xtrue])

# Wii target directory
AM_COND_IF([BUILD_RVL],[
AC_SUBST([wiidir], ['${prefix}/SD_BUILD'])])

AC_SUBST([plugindir], ['${libdir}/lsd/plugins'])
AM_COND_IF([BUILD_RVL],[
AC_SUBST([webplugindir], ['${wiidir}/lsd/webplugins'])],[
AC_SUBST([webplugindir], ['${datadir}/lsd/webplugins'])])
