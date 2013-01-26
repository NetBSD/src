#	$NetBSD: Makefile.inc,v 1.4 2013/01/26 00:20:40 christos Exp $
#

MSDOS=	${NETBSDSRCDIR}/sys/fs/msdosfs
MSDOS_NEWFS=	${NETBSDSRCDIR}/sbin/newfs_msdos

.PATH:	${.CURDIR}/msdos ${MSDOS} ${MSDOS_NEWFS}

CPPFLAGS+= -DMSDOS_EI -I${MSDOS} -I${MSDOS_NEWFS} -I${NETBSDSRCDIR}/sys

SRCS+= mkfs_msdos.c msdosfs_fat.c msdosfs_conv.c msdosfs_vfsops.c
SRCS+= msdosfs_lookup.c msdosfs_denode.c msdosfs_vnops.c
