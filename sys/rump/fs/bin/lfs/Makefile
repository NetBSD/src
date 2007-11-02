#	$NetBSD: Makefile,v 1.2.14.2 2007/11/02 12:43:25 joerg Exp $
#

PROG=		lfs

LDADD+=		${RUMPFSLD_LFS} ${RUMPFSLD_UFS} ${RUMPFSLD_FFS}
DPADD+=		${RUMPFSDP_LFS} ${RUMPFSDP_UFS} ${RUMPFSDP_FFS}

RUMPFS_WANT+=	ufs ffs

.include <bsd.prog.mk>
