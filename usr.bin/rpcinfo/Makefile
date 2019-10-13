#	$NetBSD: Makefile,v 1.16 2019/10/13 07:28:16 mrg Exp $
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

COPTS.rpcinfo.c+=	${GCC_NO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
