#	$NetBSD$

.PATH:  ${.CURDIR}/../../../../miscfs/fullfs

LIB=	rumpfs_full
COMMENT=Disk-full simulation file system

SRCS=	full_vfsops.c full_vnops.c

CWARNFLAGS.clang+=	-Wno-conversion

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
