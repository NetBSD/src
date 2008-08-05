#	$NetBSD: Makefile,v 1.10 2008/08/05 20:57:45 pooka Exp $
#
#	Id: Makefile,v 1.1.1.1 1999/02/03 03:51:19 semenu Exp 
#

.include <bsd.own.mk>

PROG=	mount_ntfs
SRCS=	mount_ntfs.c fattr.c pathadj.c
MAN=	mount_ntfs.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
