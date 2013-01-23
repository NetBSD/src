#	$NetBSD: Makefile.inc,v 1.2 2013/01/23 21:03:15 christos Exp $
#

MSDOS=	${NETBSDSRCDIR}/sys/fs/msdosfs
MSDOS_NEWFS=	${NETBSDSRCDIR}/sbin/newfs_msdos

.PATH:	${.CURDIR}/v7fs ${MSDOS} ${MSDOS_NEWFS}

CPPFLAGS+= -DMSDOS_EI -I${MSDOS} -I${MSDOS_NEWFS}
