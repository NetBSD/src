#	$NetBSD: Makefile,v 1.8 2002/01/01 00:27:37 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/utils/tfmtodit
.PATH:		${DIST}

PROG=		tfmtodit
PROG_CXX=
SRCS=		tfmtodit.cc
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
