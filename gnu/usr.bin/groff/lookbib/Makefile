#	$NetBSD: Makefile,v 1.9 2002/01/01 01:38:28 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/utils/lookbib
.PATH:		${DIST}

PROG_CXX=	lookbib
SRCS=		lookbib.cc
LDADD+= 	$(LIBBIB) $(LIBGROFF) -lm
DPADD+= 	$(LIBBIB) $(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
