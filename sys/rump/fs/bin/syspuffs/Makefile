#	$NetBSD: Makefile,v 1.1.4.2 2008/01/09 01:57:56 matt Exp $
#

PROG=		syspuffs

LDADD+=		${RUMPFSLD_SYSPUFFS}
DPADD+=		${RUMPFSDP_SYSPUFFS}

CPPFLAGS+=	-I${RUMPFSLIBDIR}/libsyspuffs

.include <bsd.prog.mk>
