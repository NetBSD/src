#	$NetBSD: Makefile,v 1.7 2002/09/18 14:00:40 lukem Exp $
#	from: @(#)Makefile	5.2 (Berkeley) 5/11/90

.include <bsd.own.mk>

PROG=	rpcinfo
MAN=	rpcinfo.8
SRCS=	rpcinfo.c rpc_generic.c

LIBCDIR=	${NETBSDSRCDIR}/lib/libc
LIBCRPCDIR=	${LIBCDIR}/rpc
LIBCINCLUDE=	${LIBCDIR}/include

CPPFLAGS+=	-I${LIBCRPCDIR} -I${LIBCINCLUDE} -DPORTMAP -DINET6

.PATH:	${LIBCRPCDIR}

.include <bsd.prog.mk>
