#	$NetBSD: Makefile,v 1.10 2002/08/19 10:16:59 lukem Exp $
#	@(#)Makefile	8.4 (Berkeley) 7/13/94

.include <bsd.own.mk>

PROG=	mount_union
SRCS=	mount_union.c getmntopts.c
MAN=	mount_union.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${NETBSDSRCDIR}/sys -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
