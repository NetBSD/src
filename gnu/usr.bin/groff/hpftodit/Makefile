#	$NetBSD: Makefile,v 1.6 2002/01/01 01:38:27 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/utils/hpftodit
.PATH:		${DIST}

PROG_CXX=	hpftodit
SRCS=		hpftodit.cc
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
