#	$NetBSD: Makefile,v 1.10 2001/04/19 14:41:20 wiz Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/devices/grotty
.PATH:		${DIST}

PROG=		grotty
SRCS=		tty.cc
LDADD+=		$(LIBDRIVER) $(LIBGROFF) -lm
DPADD+=		$(LIBDRIVER) $(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
