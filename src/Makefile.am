# Note: plugindir is set in configure

plugin_LTLIBRARIES = libgstlocal.la

libgstlocal_la_SOURCES = \
        gstlocal.c \
        gstlocalsurface.h \
        gstlocalsurface.c \
        gstlocalsink.h \
        gstlocalsink.c

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstlocal_la_CFLAGS = $(GST_CFLAGS)
libgstlocal_la_LIBADD = \
        $(GST_LIBS) \
        $(GST_PLUGINS_BASE_LIBS) \
        -lgstvideo-1.0 \
        -lgstaudio-1.0
libgstlocal_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstlocal_la_LIBTOOLFLAGS = --tag=disable-static
