NULL =

AM_CPPFLAGS = \
	-DLIBEXECDIR=\"$(libexecdir)\" \
	$(NULL)

libexec_PROGRAMS = \
	xdg-content-chooser \
	xdg-content-portal \
	$(NULL)

dbus_built_sources = xdp-dbus.c xdp-dbus.h
BUILT_SOURCES = $(dbus_built_sources)

$(dbus_built_sources): Makefile.am org.freedesktop.portal.content.xml
	$(AM_V_GEN) $(GDBUS_CODEGEN) 			\
	--interface-prefix org.freedesktop.portal.	\
	--c-namespace XdpDbus				\
	--generate-c-code xdp-dbus			\
	$(srcdir)/org.freedesktop.portal.content.xml	\
	$(NULL)

%.service: %.service.in config.log
	$(AM_V_GEN) $(SED) -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

servicedir = $(DBUS_SERVICE_DIR)
service_in_files = org.freedesktop.portal.ContentPortal.service.in
service_DATA = org.freedesktop.portal.ContentPortal.service

xdg_content_portal_SOURCES = \
	xdp-main.c		\
	$(dbus_built_sources)	\
	xdp-error.h		\
	xdp-error.c		\
	xdp-util.h		\
	xdp-util.c		\
	$(NULL)

xdg_content_portal_LDADD = $(BASE_LIBS)
xdg_content_portal_CFLAGS = $(BASE_CFLAGS)

cc_resource_files = $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/cc-content-chooser.gresource.xml)

cc-resources.h: cc-content-chooser.gresource.xml $(cc_resource_files)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) $< \
		--target=$@ --sourcedir=$(srcdir) --c-name cc --generate-header

cc-resources.c: cc-content-chooser.gresource.xml $(cc_resource_files)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) $< \
		--target=$@ --sourcedir=$(srcdir) --c-name cc --generate-source

xdg_content_chooser_SOURCES = \
	cc-main.c		\
	cc-resources.h		\
	cc-resources.c		\
	cc-content-chooser.h	\
	cc-content-chooser.c	\
	$(NULL)

xdg_content_chooser_LDADD = $(GTK_LIBS)
xdg_content_chooser_CFLAGS = $(GTK_CFLAGS)

