#	$Id: Makefile,v 1.9 1995/01/15 21:59:34 jtc Exp $

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
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 $(.CURDIR)/hyphen.us \
		$(DESTDIR)$(TMACDIR)/hyphen.us

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
