#	$NetBSD: Makefile,v 1.1.6.2 2008/01/21 09:47:39 yamt Exp $
#

PROG=		syspuffs

LDADD+=		${RUMPFSLD_SYSPUFFS}
DPADD+=		${RUMPFSDP_SYSPUFFS}

CPPFLAGS+=	-I${RUMPFSLIBDIR}/libsyspuffs

.include <bsd.prog.mk>
