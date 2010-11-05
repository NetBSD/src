#	$NetBSD: Makefile,v 1.19 2010/11/05 15:55:23 pooka Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>
#CFLAGS+=-g

PROG=	sysctl
MAN=	sysctl.8

SRCS=	sysctl.c

.ifdef RUMP_ACTION
.PATH:	${.CURDIR}/../../lib/libc/gen
CPPFLAGS+= -DRUMP_ACTION
LDADD+=	-lrumpclient
SRCS+=	sysctlgetmibinfo.c
.endif

.include <bsd.prog.mk>
