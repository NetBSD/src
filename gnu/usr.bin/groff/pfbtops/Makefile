#	$NetBSD: Makefile,v 1.1.1.1 2001/04/19 14:33:14 wiz Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/utils/pfbtops
.PATH:		${DIST}

PROG=		pfbtops
LDADD+=		$(LIBGROFF)
DPADD+=		$(LIBGROFF)

.include <bsd.prog.mk>
