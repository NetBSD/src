#	$NetBSD: Makefile,v 1.7 2001/04/19 14:41:29 wiz Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/utils/lkbib
.PATH:		${DIST}

PROG=		lkbib
SRCS=		lkbib.cc
LDADD+= 	$(LIBBIB) $(LIBGROFF) -lm
DPADD+= 	$(LIBBIB) $(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
