#	$NetBSD: Makefile,v 1.8 2002/01/01 00:27:36 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/utils/lkbib
.PATH:		${DIST}

PROG=		lkbib
PROG_CXX=
SRCS=		lkbib.cc
LDADD+= 	$(LIBBIB) $(LIBGROFF) -lm
DPADD+= 	$(LIBBIB) $(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
