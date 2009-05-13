#	$NetBSD: Makefile,v 1.12.20.1 2009/05/13 19:20:03 jym Exp $
#	from: @(#)Makefile	5.2 (Berkeley) 5/11/90

WARNS?=	2	# XXX -Wcast-qual issues

.include <bsd.own.mk>

USE_FORT?= yes	# network client

PROG=	rpcinfo
MAN=	rpcinfo.8
SRCS=	rpcinfo.c

CPPFLAGS+=	-DPORTMAP

.if (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
.endif

.PATH:	${LIBCRPCDIR}

.include <bsd.prog.mk>
