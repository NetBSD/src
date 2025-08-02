include $(top_srcdir)/Makefile.top

SUBDIRS = . lib doc

# build libtest before fuzz/* and bin/tests
SUBDIRS += tests

# run fuzz tests before system tests
SUBDIRS += fuzz bin

BUILT_SOURCES += bind.keys.h
CLEANFILES += bind.keys.h

bind.keys.h: bind.keys Makefile
	${PERL} ${top_srcdir}/util/bindkeys.pl ${top_srcdir}/bind.keys > $@

.PHONY: doc

EXTRA_DIST =			\
	bind.keys		\
	util/bindkeys.pl	\
	util/dtrace.sh		\
	contrib			\
	COPYRIGHT		\
	LICENSE			\
	*.md

dist-hook:
	find $(distdir) -type f -name .gitignore -delete
	git rev-parse --short HEAD | cut -b1-7 > $(distdir)/srcid
