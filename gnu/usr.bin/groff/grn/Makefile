#	$NetBSD: Makefile,v 1.2 2002/01/01 00:27:34 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/preproc/grn
.PATH:		${DIST}

PROG=		grn
PROG_CXX=
SRCS=		hdb.cc hgraph.cc hpoint.cc main.cc
CPPFLAGS+=	-I${DIST}
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) ${LIBM}

.include <bsd.prog.mk>
