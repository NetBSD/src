#	$NetBSD: Makefile,v 1.6 2002/01/01 01:38:27 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/devices/grolj4
.PATH:		${DIST}

PROG_CXX=	grolj4
SRCS=		lj4.cc
LDADD+=		$(LIBDRIVER) $(LIBGROFF) -lm
DPADD+=		$(LIBDRIVER) $(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
