#	$NetBSD: Makefile,v 1.7 2001/04/19 14:41:17 wiz Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/devices/grodvi
.PATH:		${DIST}

PROG=		grodvi
SRCS=		dvi.cc
LDADD+=		$(LIBDRIVER) $(LIBGROFF) -lm
DPADD+=		$(LIBDRIVER) $(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
