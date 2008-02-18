#	$NetBSD: Makefile,v 1.1.8.2 2008/02/18 21:07:19 mjf Exp $
#

PROG=		syspuffs

LDADD+=		${RUMPFSLD_SYSPUFFS}
DPADD+=		${RUMPFSDP_SYSPUFFS}

CPPFLAGS+=	-I${RUMPFSLIBDIR}/libsyspuffs

.include <bsd.prog.mk>
