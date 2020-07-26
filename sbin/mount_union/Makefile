#	$NetBSD: Makefile,v 1.15 2020/07/26 08:20:23 mlelstv Exp $
#	@(#)Makefile	8.4 (Berkeley) 7/13/94

.include <bsd.own.mk>

PROG=	mount_union
SRCS=	mount_union.c pathadj.c
MAN=	mount_union.8

MOUNT=  ${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${NETBSDSRCDIR}/sys -I${MOUNT}
.PATH:  ${MOUNT}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
