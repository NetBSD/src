#	$NetBSD: Makefile,v 1.5.10.1 2000/06/23 16:39:53 minoura Exp $
#	from: @(#)Makefile	5.2 (Berkeley) 5/11/90

PROG=	rpcinfo
MAN=	rpcinfo.8
SRCS=	rpcinfo.c rpc_generic.c

LIBCDIR=	${.CURDIR}/../../lib/libc
LIBCRPCDIR=	${LIBCDIR}/rpc
LIBCINCLUDE=	${LIBCDIR}/include

CPPFLAGS+=	-I${LIBCRPCDIR} -I${LIBCINCLUDE} -DPORTMAP -DINET6

.PATH:	${LIBCRPCDIR}

.include <bsd.prog.mk>
