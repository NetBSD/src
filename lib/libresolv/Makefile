#	$NetBSD: Makefile,v 1.7 1999/07/01 18:20:13 itojun Exp $
#	from: @(#)Makefile	5.1 (Berkeley) 6/5/90

LIB=resolv
CPPFLAGS+=-DDEBUG -DLIBC_SCCS -I${.CURDIR}/../libc/include
CPPFLAGS+=-DINET6

.PATH:	${.CURDIR}/../libc/net

SRCS=gethnamaddr.c res_mkquery.c res_query.c res_send.c

.include <bsd.lib.mk>
