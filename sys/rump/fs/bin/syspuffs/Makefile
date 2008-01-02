#	$NetBSD: Makefile,v 1.1 2008/01/02 18:15:12 pooka Exp $
#

PROG=		syspuffs

LDADD+=		${RUMPFSLD_SYSPUFFS}
DPADD+=		${RUMPFSDP_SYSPUFFS}

CPPFLAGS+=	-I${RUMPFSLIBDIR}/libsyspuffs

.include <bsd.prog.mk>
