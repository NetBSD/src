#	$NetBSD: Makefile,v 1.7 2001/04/19 14:41:42 wiz Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/utils/tfmtodit
.PATH:		${DIST}

PROG=		tfmtodit
SRCS=		tfmtodit.cc
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
