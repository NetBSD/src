#	$NetBSD: Makefile.inc,v 1.1 2013/01/23 20:46:39 christos Exp $
#

MSDOS=	${NETBSDSRCDIR}/sys/fs/msdosfs
NEWFS=	${NETBSDSRCDIR}/sbin/newfs_msdos

.PATH:	${.CURDIR}/v7fs ${MSDOS} ${NEWFS}

CPPFLAGS+= -DMSDOS_EI -I${MSDOS} -I${NEWFS}
