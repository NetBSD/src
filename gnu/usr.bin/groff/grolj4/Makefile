#	$NetBSD: Makefile,v 1.1.1.1 2001/04/19 14:33:07 wiz Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/devices/grolj4
.PATH:		${DIST}

PROG=		grolj4
SRCS=		lj4.cc
LDADD+=		$(LIBDRIVER) $(LIBGROFF) -lm
DPADD+=		$(LIBDRIVER) $(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
