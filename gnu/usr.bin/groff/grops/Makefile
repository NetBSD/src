#	$NetBSD: Makefile,v 1.1.1.2 2001/04/19 14:33:07 wiz Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/devices/grops
.PATH:		${DIST}

PROG=		grops
SRCS=		ps.cc psrm.cc
LDADD+=		$(LIBDRIVER) $(LIBGROFF) -lm
DPADD+=		$(LIBDRIVER) $(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
