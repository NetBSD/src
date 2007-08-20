#	$NetBSD: Makefile,v 1.2.4.2 2007/08/20 22:07:14 ad Exp $
#

PROG=		lfs

LDADD+=		${RUMPFSLD_LFS} ${RUMPFSLD_UFS} ${RUMPFSLD_FFS}
DPADD+=		${RUMPFSDP_LFS} ${RUMPFSDP_UFS} ${RUMPFSDP_FFS}

RUMPFS_WANT+=	ufs ffs

.include <bsd.prog.mk>
