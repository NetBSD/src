#	$NetBSD: Makefile.inc,v 1.5.4.2 2013/02/25 00:30:45 tls Exp $
#

MSDOS=	${NETBSDSRCDIR}/sys/fs/msdosfs
MSDOS_NEWFS=	${NETBSDSRCDIR}/sbin/newfs_msdos

.PATH:	${.CURDIR}/msdos ${MSDOS} ${MSDOS_NEWFS}

CPPFLAGS+= -DMSDOS_EI -I${MSDOS} -I${MSDOS_NEWFS}
.if !defined(HOSTPROGNAME)
CPPFLAGS+= -I${NETBSDSRCDIR}/sys
.endif

SRCS+= mkfs_msdos.c msdosfs_fat.c msdosfs_conv.c msdosfs_vfsops.c
SRCS+= msdosfs_lookup.c msdosfs_denode.c msdosfs_vnops.c
