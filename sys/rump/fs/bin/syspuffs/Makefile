#	$NetBSD: Makefile,v 1.1.2.2 2008/01/02 21:57:47 bouyer Exp $
#

PROG=		syspuffs

LDADD+=		${RUMPFSLD_SYSPUFFS}
DPADD+=		${RUMPFSDP_SYSPUFFS}

CPPFLAGS+=	-I${RUMPFSLIBDIR}/libsyspuffs

.include <bsd.prog.mk>
