#	$NetBSD: Makefile,v 1.3 2020/07/26 08:20:22 mlelstv Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_autofs
SRCS=	mount_autofs.c pathadj.c
MAN=	mount_autofs.8

MOUNT=  ${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${NETBSDSRCDIR}/sys -I${MOUNT}
.PATH:  ${MOUNT}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
