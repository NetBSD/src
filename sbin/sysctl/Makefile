#	$NetBSD: Makefile,v 1.20.12.1 2013/02/25 00:28:11 tls Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>
#DBG=-g

RUMPPRG=sysctl
MAN=	sysctl.8

SRCS=	sysctl.c

.PATH:	${.CURDIR}/../../lib/libc/gen
CPPFLAGS+= -DRUMP_ACTION
RUMPSRCS+= sysctlbyname.c sysctlgetmibinfo.c

.include <bsd.prog.mk>
