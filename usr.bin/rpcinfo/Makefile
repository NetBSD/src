#	$NetBSD: Makefile,v 1.6 2000/06/02 23:19:38 fvdl Exp $
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
