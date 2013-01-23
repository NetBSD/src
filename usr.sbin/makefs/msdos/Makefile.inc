#	$NetBSD: Makefile.inc,v 1.3 2013/01/23 22:47:18 christos Exp $
#

MSDOS=	${NETBSDSRCDIR}/sys/fs/msdosfs
MSDOS_NEWFS=	${NETBSDSRCDIR}/sbin/newfs_msdos

.PATH:	${.CURDIR}/msdos ${MSDOS} ${MSDOS_NEWFS}

CPPFLAGS+= -DMSDOS_EI -I${MSDOS} -I${MSDOS_NEWFS}

SRCS+= mkfs_msdos.c
