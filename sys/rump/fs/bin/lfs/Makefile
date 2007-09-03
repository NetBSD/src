#	$NetBSD: Makefile,v 1.2.8.2 2007/09/03 14:44:44 yamt Exp $
#

PROG=		lfs

LDADD+=		${RUMPFSLD_LFS} ${RUMPFSLD_UFS} ${RUMPFSLD_FFS}
DPADD+=		${RUMPFSDP_LFS} ${RUMPFSDP_UFS} ${RUMPFSDP_FFS}

RUMPFS_WANT+=	ufs ffs

.include <bsd.prog.mk>
