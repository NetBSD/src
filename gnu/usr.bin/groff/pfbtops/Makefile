#	$NetBSD: Makefile,v 1.8 2002/01/01 05:16:18 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/utils/pfbtops
.PATH:		${DIST}

PROG=		pfbtops
LDADD+=		$(LIBGROFF)
DPADD+=		$(LIBGROFF)

.include <bsd.prog.mk>
