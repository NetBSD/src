#	$NetBSD: Makefile,v 1.9 2002/01/01 01:38:29 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/utils/tfmtodit
.PATH:		${DIST}

PROG_CXX=	tfmtodit
SRCS=		tfmtodit.cc
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
