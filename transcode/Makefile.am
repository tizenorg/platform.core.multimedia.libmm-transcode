ACLOCAL_AMFLAGS='-I m4'

lib_LTLIBRARIES = libmm_transcode.la

installmm_transcodedir = $(includedir)/mmf
installmm_transcode_HEADERS = include/mm_transcode.h
noinst_HEADERS = include/mm_transcode_internal.h

libmm_transcode_la_SOURCES = mm_transcode.c \
                             mm_transcode_pipeline.c \
                             mm_transcode_seek.c \
                             mm_transcode_codec.c

libmm_transcode_la_CFLAGS = -I$(srcdir)/include \
			     $(MMCOMMON_CFLAGS) \
			     $(MMFILEINFO_CFLAGS) \
                             $(MMTA_CFLAGS) -DGST_EXT_TIME_ANALYSIS \
			     $(GLIB_CFLAGS) \
			     $(GST_CFLAGS) \
			     $(GSTAPP_CFLAGS)  \
                             $(GST_PLUGIN_BASE_CFLAGS) \
			     $(GST_INTERFACE_CFLAGS) \
                             $(GSTPBUTILS_CFLAGS) \
                             $(MMLOG_CFLAGS) -DMMF_LOG_OWNER=0x300000 -DMMF_DEBUG_PREFIX=\"MMF-VIDEOEDIT\"

libmm_transcode_la_LIBADD = $(MMCOMMON_LIBS) \
			    $(MMFILEINFO_LIBS) \
                            $(MMTA_LIBS) \
			    $(GLIB_LIBS) \
			    $(GST_LIBS) \
			    $(GSTAPP_LIBS) \
                            $(GST_PLUGIN_BASE_LIBS) \
			    $(GST_INTERFACE_LIBS) \
                            $(GSTPBUTILS_LIBS) \
			    $(MMLOG_LIBS)

pcfiles = mm-transcode.pc
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(pcfiles)
EXTRA_DIST = $(pcfiles)
SUBDIRS = . test
