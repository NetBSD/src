#	$NetBSD: Makefile,v 1.9 2007/05/26 22:58:43 christos Exp $
#	from: @(#)Makefile	5.2 (Berkeley) 5/11/90

.include <bsd.own.mk>

PROG=	rpcinfo
MAN=	rpcinfo.8
SRCS=	rpcinfo.c

LIBCDIR=	${NETBSDSRCDIR}/lib/libc
LIBCRPCDIR=	${LIBCDIR}/rpc
LIBCINCLUDE=	${LIBCDIR}/include

CPPFLAGS+=	-I${LIBCRPCDIR} -I${LIBCINCLUDE} -DPORTMAP

.if (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
.endif

.PATH:	${LIBCRPCDIR}

.include <bsd.prog.mk>
