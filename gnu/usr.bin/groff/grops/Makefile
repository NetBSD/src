#	$NetBSD: Makefile,v 1.11 2002/01/01 00:27:35 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/devices/grops
.PATH:		${DIST}

PROG=		grops
PROG_CXX=
SRCS=		ps.cc psrm.cc
LDADD+=		$(LIBDRIVER) $(LIBGROFF) -lm
DPADD+=		$(LIBDRIVER) $(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
