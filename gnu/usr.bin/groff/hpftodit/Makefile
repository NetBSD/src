#	$NetBSD: Makefile,v 1.5 2002/01/01 00:27:35 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/utils/hpftodit
.PATH:		${DIST}

PROG=		hpftodit
PROG_CXX=
SRCS=		hpftodit.cc
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
