#	$NetBSD: Makefile,v 1.6 1997/10/23 03:20:40 lukem Exp $
#	from: @(#)Makefile	5.1 (Berkeley) 6/5/90

LIB=resolv
CPPFLAGS+=-DDEBUG -DLIBC_SCCS -I${.CURDIR}/../libc/include

.PATH:	${.CURDIR}/../libc/net

SRCS=gethnamaddr.c res_mkquery.c res_query.c res_send.c

.include <bsd.lib.mk>
