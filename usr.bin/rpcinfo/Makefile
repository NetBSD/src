#	$NetBSD: Makefile,v 1.10 2007/05/26 23:00:22 christos Exp $
#	from: @(#)Makefile	5.2 (Berkeley) 5/11/90

.include <bsd.own.mk>

PROG=	rpcinfo
MAN=	rpcinfo.8
SRCS=	rpcinfo.c

CPPFLAGS+=	-DPORTMAP

.if (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
.endif

.PATH:	${LIBCRPCDIR}

.include <bsd.prog.mk>
