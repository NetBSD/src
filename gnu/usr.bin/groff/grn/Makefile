#	$NetBSD: Makefile,v 1.1.1.1 2001/04/19 14:33:14 wiz Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/preproc/grn
.PATH:		${DIST}

PROG=		grn
SRCS=		hdb.cc hgraph.cc hpoint.cc main.cc
CPPFLAGS+=	-I${DIST}
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) ${LIBM}

.include <bsd.prog.mk>
