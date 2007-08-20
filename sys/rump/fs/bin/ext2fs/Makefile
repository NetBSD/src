#	$NetBSD: Makefile,v 1.1.4.2 2007/08/20 22:07:11 ad Exp $
#

PROG=		ext2fs

LDADD+=		${RUMPFSLD_EXT2FS} ${RUMPFSLD_UFS} ${RUMPFSLD_FFS}
DPADD+=		${RUMPFSDP_EXT2FS} ${RUMPFSDP_UFS} ${RUMPFSDP_FFS}

RUMPFS_WANT+=	ufs ffs

.include <bsd.prog.mk>
