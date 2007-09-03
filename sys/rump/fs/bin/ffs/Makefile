#	$NetBSD: Makefile,v 1.1.8.2 2007/09/03 14:44:38 yamt Exp $
#

PROG=		ffs

LDADD+=		${RUMPFSLD_FFS} ${RUMPFSLD_UFS}
DPADD+=		${RUMPFSDP_FFS} ${RUMPFSDP_UFS}

RUMPFS_WANT+=	ufs

.include <bsd.prog.mk>
