#	$NetBSD: Makefile,v 1.2 2002/01/01 00:27:34 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/devices/grolbp
.PATH:		${DIST}

PROG=		grolbp
PROG_CXX=
SRCS=		lbp.cc
LDADD+=		$(LIBDRIVER) $(LIBGROFF) -lm
DPADD+=		$(LIBDRIVER) $(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
