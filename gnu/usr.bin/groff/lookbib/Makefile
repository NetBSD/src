#	$NetBSD: Makefile,v 1.8 2002/01/01 00:27:36 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/utils/lookbib
.PATH:		${DIST}

PROG=		lookbib
PROG_CXX=
SRCS=		lookbib.cc
LDADD+= 	$(LIBBIB) $(LIBGROFF) -lm
DPADD+= 	$(LIBBIB) $(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
