#	$NetBSD: Makefile,v 1.1.8.2 2007/09/03 14:44:33 yamt Exp $
#

PROG=		ext2fs

LDADD+=		${RUMPFSLD_EXT2FS} ${RUMPFSLD_UFS} ${RUMPFSLD_FFS}
DPADD+=		${RUMPFSDP_EXT2FS} ${RUMPFSDP_UFS} ${RUMPFSDP_FFS}

RUMPFS_WANT+=	ufs ffs

.include <bsd.prog.mk>
