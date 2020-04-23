#	$NetBSD: Makefile,v 1.20 2020/04/23 04:21:13 christos Exp $
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

COPTS.umount.c+=	${GCC_NO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
