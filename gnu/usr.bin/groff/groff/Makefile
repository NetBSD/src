#	$NetBSD: Makefile,v 1.11 2002/01/01 01:38:26 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/roff/groff
.PATH:		${DIST}

PROG_CXX=	groff
SRCS=		groff.cc pipeline.c
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
