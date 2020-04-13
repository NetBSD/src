#	$NetBSD: Makefile,v 1.18.28.1 2020/04/13 08:03:24 martin Exp $
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

COPTS.umount.c+=	${GCC_NO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
