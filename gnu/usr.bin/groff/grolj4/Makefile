#	$NetBSD: Makefile,v 1.5 2002/01/01 00:27:35 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/devices/grolj4
.PATH:		${DIST}

PROG=		grolj4
PROG_CXX=
SRCS=		lj4.cc
LDADD+=		$(LIBDRIVER) $(LIBGROFF) -lm
DPADD+=		$(LIBDRIVER) $(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
