pkgdir = $(libdir)/location/module
if WPS
pkg_LTLIBRARIES = libgps.la libwps.la
else
pkg_LTLIBRARIES = libgps.la
endif

libgps_la_SOURCES = gps_module.c
libgps_la_CFLAGS = \
				 -D_GNU_SOURCE \
				 -fPIC\
				 $(MODULE_CFLAGS)
libgps_la_LIBADD = \
				 -lm\
				 -ldl\
				 $(MODULE_LIBS)

libwps_la_SOURCES = nps_module.c
libwps_la_CFLAGS = \
				 -D_GNU_SOURCE \
				 -fPIC\
				 $(MODULE_CFLAGS)
libwps_la_LIBADD = \
				 -lm\
				 -ldl\
				 $(MODULE_LIBS)


