AUTOMAKE_OPTIONS = 1.5 foreign no-dependencies
mkinstalldirs = $(SHELL) @global_top_auxdir@/mkinstalldirs
bin_PROGRAMS = usey
usey_SOURCES = usey.c
LDADD = @LIBRPATHY@

check: all
	./usey
