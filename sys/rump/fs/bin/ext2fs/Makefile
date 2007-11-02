#	$NetBSD: Makefile,v 1.1.14.2 2007/11/02 12:43:20 joerg Exp $
#

PROG=		ext2fs

LDADD+=		${RUMPFSLD_EXT2FS} ${RUMPFSLD_UFS} ${RUMPFSLD_FFS}
DPADD+=		${RUMPFSDP_EXT2FS} ${RUMPFSDP_UFS} ${RUMPFSDP_FFS}

RUMPFS_WANT+=	ufs ffs

.include <bsd.prog.mk>
