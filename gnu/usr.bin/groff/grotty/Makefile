#	$NetBSD: Makefile,v 1.11 2002/01/01 00:27:35 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/devices/grotty
.PATH:		${DIST}

PROG=		grotty
PROG_CXX=
SRCS=		tty.cc
LDADD+=		$(LIBDRIVER) $(LIBGROFF) -lm
DPADD+=		$(LIBDRIVER) $(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
