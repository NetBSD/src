#	$NetBSD: Makefile,v 1.11 2002/09/21 18:43:38 christos Exp $
#	@(#)Makefile	8.4 (Berkeley) 7/13/94

.include <bsd.own.mk>

PROG=	mount_union
SRCS=	mount_union.c getmntopts.c
MAN=	mount_union.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${NETBSDSRCDIR}/sys -I${MOUNT}
.PATH:	${MOUNT}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
