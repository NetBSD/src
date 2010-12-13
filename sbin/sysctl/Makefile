#	$NetBSD: Makefile,v 1.20 2010/12/13 17:47:40 pooka Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>
#CFLAGS+=-g

RUMPPRG=sysctl
MAN=	sysctl.8

SRCS=	sysctl.c

.PATH:	${.CURDIR}/../../lib/libc/gen
CPPFLAGS+= -DRUMP_ACTION
RUMPSRCS+= sysctlbyname.c sysctlgetmibinfo.c

.include <bsd.prog.mk>
