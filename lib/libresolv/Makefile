#	$NetBSD: Makefile,v 1.9 2002/08/19 09:41:33 lukem Exp $
#	from: @(#)Makefile	5.1 (Berkeley) 6/5/90

.include <bsd.own.mk>

LIB=resolv
CPPFLAGS+=-DDEBUG -DLIBC_SCCS -I${NETBSDSRCDIR}/lib/libc/include
CPPFLAGS+=-DRESOLVSORT
CPPFLAGS+=-DINET6

.PATH:	${NETBSDSRCDIR}/lib/libc/net

SRCS=gethnamaddr.c res_mkquery.c res_query.c res_send.c

.include <bsd.lib.mk>
