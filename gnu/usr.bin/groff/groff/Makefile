#	$NetBSD: Makefile,v 1.9 2001/04/19 14:41:17 wiz Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/roff/groff
.PATH:		${DIST}

PROG=		groff
SRCS=		groff.cc pipeline.c
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
