#	$NetBSD: Makefile,v 1.9 2002/08/19 10:16:58 lukem Exp $
#	@(#)Makefile	8.3 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_null
SRCS=	mount_null.c getmntopts.c
MAN=	mount_null.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${NETBSDSRCDIR}/sys -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
