#	$NetBSD: Makefile,v 1.3 2002/01/01 01:38:27 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/devices/grolbp
.PATH:		${DIST}

PROG_CXX=	grolbp
SRCS=		lbp.cc
LDADD+=		$(LIBDRIVER) $(LIBGROFF) -lm
DPADD+=		$(LIBDRIVER) $(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
