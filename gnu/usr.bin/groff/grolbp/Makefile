#	$NetBSD: Makefile,v 1.1.1.1 2001/04/19 14:33:14 wiz Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/devices/grolbp
.PATH:		${DIST}

PROG=		grolbp
SRCS=		lbp.cc
LDADD+=		$(LIBDRIVER) $(LIBGROFF) -lm
DPADD+=		$(LIBDRIVER) $(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
