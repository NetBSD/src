#	$NetBSD: Makefile,v 1.7 2002/01/01 01:38:28 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/utils/pfbtops
.PATH:		${DIST}

PROG_CXX=	pfbtops
LDADD+=		$(LIBGROFF)
DPADD+=		$(LIBGROFF)

.include <bsd.prog.mk>
