#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([libmm-transcode],[0.1])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

# Checks for programs.
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL

# Checks for libraries.
# FIXME: Replace `main' with a function in `-liniparser':
# AC_CHECK_LIB([iniparser], [main])
# FIXME: Replace `main' with a function in `-ljpeg':
# AC_CHECK_LIB([jpeg], [main])
# FIXME: Replace `main' with a function in `-lrt':
# AC_CHECK_LIB([rt], [main])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h memory.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset strdup])

PKG_CHECK_MODULES(MMCOMMON, mm-common)
AC_SUBST(MMCOMMON_CFLAGS)
AC_SUBST(MMCOMMON_LIBS)

PKG_CHECK_MODULES(MMLOG, mm-log)
AC_SUBST(MMLOG_CFLAGS)
AC_SUBST(MMLOG_LIBS)

PKG_CHECK_MODULES(MMTA, mm-ta)
AC_SUBST(MMTA_CFLAGS)
AC_SUBST(MMTA_LIBS)

PKG_CHECK_MODULES(MMFILEINFO, mm-fileinfo)
AC_SUBST(MMFILEINFO_CFLAGS)
AC_SUBST(MMFILEINFO_LIBS)

PKG_CHECK_MODULES(GLIB, glib-2.0)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

PKG_CHECK_MODULES(GST, gstreamer-0.10 >= 0.10)
AC_SUBST(GST_CFLAGS)
AC_SUBST(GST_LIBS)

PKG_CHECK_MODULES(GST_PLUGIN_BASE, gstreamer-plugins-base-0.10 >= 0.10)
AC_SUBST(GST_PLUGIN_BASE_CFLAGS)
AC_SUBST(GST_PLUGIN_BASE_LIBS)

PKG_CHECK_MODULES(GST_INTERFACE, gstreamer-interfaces-0.10 >= 0.10)
AC_SUBST(GST_INTERFACE_CFLAGS)
AC_SUBST(GST_LIBS)

PKG_CHECK_MODULES(GSTAPP, gstreamer-app-0.10 >= 0.10)
AC_SUBST(GSTAPP_CFLAGS)
AC_SUBST(GSTAPP_LIBS)

PKG_CHECK_MODULES(GSTPBUTILS, gstreamer-pbutils-0.10 >= 0.10)
AC_SUBST(GSTPBUTILS_CFLAGS)
AC_SUBST(GSTPBUTILS_LIBS)

PKG_CHECK_MODULES(GMODULE, gmodule-2.0)
AC_SUBST(GMODULE_CFLAGS)
AC_SUBST(GMODULE_LIBS)

AC_CONFIG_FILES([Makefile
		 transcode/Makefile
		 transcode/test/Makefile
                 transcode/mm-transcode.pc
])
AC_OUTPUT
