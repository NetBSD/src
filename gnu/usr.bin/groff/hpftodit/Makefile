#	$NetBSD: Makefile,v 1.4 2001/04/19 14:41:21 wiz Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/utils/hpftodit
.PATH:		${DIST}

PROG=		hpftodit
SRCS=		hpftodit.cc
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
