#	$NetBSD: Makefile,v 1.2 2008/07/29 16:06:18 pooka Exp $
#

PROG=		rump_msdos

MOUNT=		${.CURDIR}/../../../sbin/mount
MOUNTMSDOS=	${.CURDIR}/../../../sbin/mount_msdos

.PATH:		${MOUNT} ${MOUNTMSDOS}
SRCS=		rump_msdos.c mount_msdos.c fattr.c

LDADD+=		-lrumpfs_msdosfs
DPADD+=		${LIBRUMPFS_MSDOSFS}
CPPFLAGS+=	-DMOUNT_NOMAIN -I${MOUNT} -I${MOUNTMSDOS}

ISRUMP=		# rump, si

.include <bsd.prog.mk>
