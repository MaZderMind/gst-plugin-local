plugin_LTLIBRARIES = libgstlocal.la

libgstlocal_la_SOURCES = \
	gstlocal.c \
	gstlocalsurface.c \
	gstlocalsink.c

noinst_HEADERS = \
	gstlocalsink.h \
	gstlocalsurface.h

libgstlocal_la_CFLAGS = \
	$(GST_PLUGINS_BAD_CFLAGS) \
	$(GST_PLUGINS_BASE_CFLAGS) \
	$(GST_BASE_CFLAGS) \
	$(GST_CFLAGS)

libgstlocal_la_LIBADD = \
	$(GST_PLUGINS_BASE_LIBS) -lgstvideo-@GST_API_VERSION@ -lgstaudio-@GST_API_VERSION@ \
	$(GST_BASE_LIBS) \
	$(GST_LIBS) \
	$(LIBM)

libgstlocal_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstlocal_la_LIBTOOLFLAGS = $(GST_PLUGIN_LIBTOOLFLAGS)
