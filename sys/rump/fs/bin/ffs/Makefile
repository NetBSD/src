#	$NetBSD: Makefile,v 1.1.14.2 2007/11/02 13:02:43 joerg Exp $
#

PROG=		ffs

LDADD+=		${RUMPFSLD_FFS} ${RUMPFSLD_UFS}
DPADD+=		${RUMPFSDP_FFS} ${RUMPFSDP_UFS}

RUMPFS_WANT+=	ufs

.include <bsd.prog.mk>
