#	$NetBSD: Makefile,v 1.3 2002/01/01 01:38:26 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/preproc/grn
.PATH:		${DIST}

PROG_CXX=	grn
SRCS=		hdb.cc hgraph.cc hpoint.cc main.cc
CPPFLAGS+=	-I${DIST}
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) ${LIBM}

.include <bsd.prog.mk>
