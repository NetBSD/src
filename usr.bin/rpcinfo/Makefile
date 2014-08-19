#	$NetBSD: Makefile,v 1.14.10.1 2014/08/20 00:05:03 tls Exp $
#	from: @(#)Makefile	5.2 (Berkeley) 5/11/90

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
