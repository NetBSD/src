#	$NetBSD: Makefile,v 1.6 2002/01/01 00:27:36 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/utils/pfbtops
.PATH:		${DIST}

PROG=		pfbtops
PROG_CXX=
LDADD+=		$(LIBGROFF)
DPADD+=		$(LIBGROFF)

.include <bsd.prog.mk>
