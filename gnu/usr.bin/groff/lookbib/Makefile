#	$NetBSD: Makefile,v 1.7 2001/04/19 14:41:30 wiz Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/utils/lookbib
.PATH:		${DIST}

PROG=		lookbib
SRCS=		lookbib.cc
LDADD+= 	$(LIBBIB) $(LIBGROFF) -lm
DPADD+= 	$(LIBBIB) $(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
