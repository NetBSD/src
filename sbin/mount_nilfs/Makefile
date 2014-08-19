#	$NetBSD: Makefile,v 1.1.12.1 2014/08/20 00:02:26 tls Exp $

.include <bsd.own.mk>

PROG=	mount_nilfs
SRCS=	mount_nilfs.c fattr.c pathadj.c
MAN=	mount_nilfs.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${MOUNT} -I${NETBSDSRCDIR}/sys
.PATH:	${MOUNT}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
