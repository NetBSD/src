#	$NetBSD: Makefile,v 1.8 2000/01/31 15:42:34 christos Exp $
#	from: @(#)Makefile	5.1 (Berkeley) 6/5/90

LIB=resolv
CPPFLAGS+=-DDEBUG -DLIBC_SCCS -I${.CURDIR}/../libc/include
CPPFLAGS+=-DRESOLVSORT
CPPFLAGS+=-DINET6

.PATH:	${.CURDIR}/../libc/net

SRCS=gethnamaddr.c res_mkquery.c res_query.c res_send.c

.include <bsd.lib.mk>
