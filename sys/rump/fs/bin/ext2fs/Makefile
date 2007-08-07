#	$NetBSD: Makefile,v 1.1 2007/08/07 21:26:36 pooka Exp $
#

PROG=		ext2fs

LDADD+=		${RUMPFSLD_EXT2FS} ${RUMPFSLD_UFS} ${RUMPFSLD_FFS}
DPADD+=		${RUMPFSDP_EXT2FS} ${RUMPFSDP_UFS} ${RUMPFSDP_FFS}

RUMPFS_WANT+=	ufs ffs

.include <bsd.prog.mk>
