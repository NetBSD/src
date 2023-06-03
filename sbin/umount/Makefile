#	$NetBSD: Makefile,v 1.21 2023/06/03 09:09:13 lukem Exp $
#	@(#)Makefile	8.4 (Berkeley) 6/22/95

.include <bsd.own.mk>

PROG=	umount
SRCS=	umount.c
MAN=	umount.8

.ifdef SMALLPROG
CPPFLAGS+= -DSMALL
.else
MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}
SRCS+=	vfslist.c
.endif
LDADD+=-lutil
DPADD+=${LIBUTIL}

COPTS.umount.c+=	${CC_WNO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
