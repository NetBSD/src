#	$NetBSD: Makefile,v 1.12 2002/01/01 01:38:27 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/devices/grops
.PATH:		${DIST}

PROG_CXX=	grops
SRCS=		ps.cc psrm.cc
LDADD+=		$(LIBDRIVER) $(LIBGROFF) -lm
DPADD+=		$(LIBDRIVER) $(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
