#	$NetBSD: Makefile,v 1.9 2002/01/01 01:38:26 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/devices/grodvi
.PATH:		${DIST}

PROG_CXX=	grodvi
SRCS=		dvi.cc
LDADD+=		$(LIBDRIVER) $(LIBGROFF) -lm
DPADD+=		$(LIBDRIVER) $(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
