#	$NetBSD: Makefile,v 1.10 2004/05/21 03:33:44 christos Exp $
#	from: @(#)Makefile	5.1 (Berkeley) 6/5/90

.include <bsd.own.mk>

LIB=resolv
CPPFLAGS+=-DLIBC_SCCS -I${NETBSDSRCDIR}/lib/libc/include -D_LIBC
CPPFLAGS+=-DINET6

.PATH:	${NETBSDSRCDIR}/lib/libc/net ${NETBSDSRCDIR}/lib/libc/resolv

SRCS=gethnamaddr.c res_mkquery.c res_query.c res_send.c

.include <bsd.lib.mk>
