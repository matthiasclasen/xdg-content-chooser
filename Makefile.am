NULL =

AM_CPPFLAGS = \
	$(NULL)

libexec_PROGRAMS = \
	xdg-content-chooser \
	$(NULL)

resource_files = $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/cc-content-chooser.gresource.xml)

cc-resources.h: cc-content-chooser.gresource.xml $(resource_files)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) $< \
		--target=$@ --sourcedir=$(srcdir) --c-name cc --generate-header

cc-resources.c: cc-content-chooser.gresource.xml $(resource_files)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) $< \
		--target=$@ --sourcedir=$(srcdir) --c-name cc --generate-source

xdg_content_chooser_SOURCES = \
	cc-main.c		\
	cc-resources.h		\
	cc-resources.c		\
	cc-content-chooser.h	\
	cc-content-chooser.c	\
	$(NULL)

xdg_content_chooser_LDADD = $(BASE_LIBS)
xdg_content_chooser_CFLAGS = $(BASE_CFLAGS)
