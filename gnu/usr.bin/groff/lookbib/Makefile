#	$NetBSD: Makefile,v 1.1.1.1 2001/04/19 14:33:14 wiz Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/utils/lookbib
.PATH:		${DIST}

PROG=		lookbib
SRCS=		lookbib.cc
LDADD+= 	$(LIBBIB) $(LIBGROFF) -lm
DPADD+= 	$(LIBBIB) $(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
