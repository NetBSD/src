#	$NetBSD: Makefile,v 1.8 2002/01/01 00:27:34 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/devices/grodvi
.PATH:		${DIST}

PROG=		grodvi
PROG_CXX=
SRCS=		dvi.cc
LDADD+=		$(LIBDRIVER) $(LIBGROFF) -lm
DPADD+=		$(LIBDRIVER) $(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
