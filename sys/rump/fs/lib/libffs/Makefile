#	$NetBSD: Makefile,v 1.23 2023/06/03 09:09:14 lukem Exp $
#

.PATH:  ${.CURDIR}/../../../../ufs/ffs ${.CURDIR}/../../../../ufs/ufs

LIB=	rumpfs_ffs
COMMENT=Berkeley Fast File System

SRCS=	ffs_alloc.c ffs_appleufs.c ffs_balloc.c ffs_bswap.c ffs_inode.c	\
	ffs_snapshot.c ffs_subr.c ffs_tables.c ffs_vfsops.c ffs_vnops.c	\
	ffs_wapbl.c ffs_quota2.c ffs_extattr.c

SRCS+=	ufs_acl.c ufs_bmap.c ufs_dirhash.c ufs_extattr.c ufs_inode.c	\
	ufs_lookup.c ufs_rename.c ufs_vfsops.c ufs_vnops.c ufs_quota.c	\
	ufs_quota2.c quota2_subr.c

CPPFLAGS+=	-DFFS_EI -DUFS_DIRHASH -DWAPBL -DAPPLE_UFS -DUFS_EXTATTR \
		-DQUOTA2 -DUFS_ACL

CWARNFLAGS.gcc+=	${CC_WNO_ADDRESS_OF_PACKED_MEMBER} ${CC_WNO_FORMAT_TRUNCATION}
CWARNFLAGS.gcc+=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 9:? -Wno-error=restrict :}

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
