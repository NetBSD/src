#	$NetBSD: Makefile,v 1.2 2010/07/26 16:15:49 pooka Exp $
#

.include <bsd.own.mk>

LIB=	vfstest
SRCS=	ext2fs.c ffs.c lfs.c msdosfs.c nfs.c puffs.c sysvbfs.c tmpfs.c

WARNS=	3

.include "nfsrpc/Makefile.inc"

LIBISPRIVATE=	# yup

# NFS client stuff
.PATH:	${NETBSDSRCDIR}/sbin/mount_nfs ${NETBSDSRCDIR}/sbin/mount
SRCS+=	getnfsargs.c pathadj.c fattr.c mount_nfs.c

CPPFLAGS+=	-I${NETBSDSRCDIR}/sbin/mount_nfs -I${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+=	-DMOUNT_NOMAIN -DMOUNTNFS_RETRYRPC=1

.include <bsd.lib.mk>
