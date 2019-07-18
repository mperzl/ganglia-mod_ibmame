AM_CFLAGS  = -I$(top_builddir)/include -I$(top_builddir)/lib -I$(top_builddir)/libmetrics

if STATIC_BUILD
noinst_LTLIBRARIES    = libmodibmame.la
libmodibmame_la_SOURCES = mod_ibmame.c 
else
pkglib_LTLIBRARIES    = modibmame.la
modibmame_la_SOURCES = mod_ibmame.c 
modibmame_la_LDFLAGS = -module -avoid-version
modibmame_la_LIBADD = $(top_builddir)/libmetrics/libmetrics.la

EXTRA_DIST = ../conf.d/ibmame.conf
endif

INCLUDES = @APR_INCLUDES@

