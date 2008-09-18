#	$NetBSD: Makefile,v 1.7.24.1 2008/09/18 04:28:26 wrstuden Exp $
#
#	$OpenBSD: Makefile,v 1.1 1996/06/27 07:20:28 downsj Exp $

.include <bsd.own.mk>

PROG=	mount_ext2fs
SRCS=	mount_ext2fs.c pathadj.c
MAN=	mount_ext2fs.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+=-I${MOUNT}
.PATH:	${MOUNT}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
