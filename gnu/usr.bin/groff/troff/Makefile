# Makefile for troff

PROG=		troff
SRCS=		env.cc node.cc input.cc div.cc symbol.cc dictionary.cc reg.cc \
		number.cc majorminor.cc
CFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBMATH)

CLEANFILES+=	majorminor.cc
majorminor.cc: $(.CURDIR)/../VERSION
	@echo Making $@
	@-rm -f $@
	@echo const char \*major_version = \
	\"`sed -e 's/^\([^.]*\)\..*$$/\1/' $(.CURDIR)/../VERSION`\"\; >$@
	@echo const char \*minor_version = \
	\"`sed -e 's/^[^.]*\.\([0-9]*\).*$$/\1/' $(.CURDIR)/../VERSION`\"\; >>$@

# XXX -- this depends on ../include/defs.h
TMACDIR=/usr/share/tmac

afterinstall:
	install -c -o bin -g bin -m 444 $(.CURDIR)/hyphen.us \
		$(DESTDIR)$(TMACDIR)/hyphen.us

.include <bsd.prog.mk>
.include "../../Makefile.inc"
.include "../Makefile.cfg"
