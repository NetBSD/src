#	$NetBSD: Makefile,v 1.10 2002/01/01 00:27:34 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/roff/groff
.PATH:		${DIST}

PROG=		groff
PROG_CXX=
SRCS=		groff.cc pipeline.c
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
