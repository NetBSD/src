#	$NetBSD: Makefile,v 1.11.50.2 2013/06/14 04:17:36 msaitoh Exp $
#	from: @(#)Makefile	5.1 (Berkeley) 6/5/90

.include <bsd.own.mk>

LIB=resolv
CPPFLAGS+=-DLIBC_SCCS -I${NETBSDSRCDIR}/lib/libc/include -D_LIBC

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

.PATH:	${NETBSDSRCDIR}/lib/libc/net ${NETBSDSRCDIR}/lib/libc/resolv

SRCS=gethnamaddr.c res_mkquery.c res_query.c res_send.c

.include <bsd.lib.mk>
