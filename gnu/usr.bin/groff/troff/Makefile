#	$NetBSD: Makefile,v 1.15 1998/09/27 17:22:06 lukem Exp $

PROG=		troff
SRCS=		env.cc node.cc input.cc div.cc symbol.cc dictionary.cc reg.cc \
		number.cc majorminor.cc
CPPFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

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

.if !defined(NOSHARE)
FILES=hyphen.us
FILESDIR=${TMACDIR}
.endif

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
