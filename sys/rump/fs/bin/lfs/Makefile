#	$NetBSD: Makefile,v 1.2 2007/08/09 09:19:30 pooka Exp $
#

PROG=		lfs

LDADD+=		${RUMPFSLD_LFS} ${RUMPFSLD_UFS} ${RUMPFSLD_FFS}
DPADD+=		${RUMPFSDP_LFS} ${RUMPFSDP_UFS} ${RUMPFSDP_FFS}

RUMPFS_WANT+=	ufs ffs

.include <bsd.prog.mk>
