#	$NetBSD: Makefile,v 1.5 1997/09/20 04:23:18 mikel Exp $
#	from: @(#)Makefile	5.1 (Berkeley) 6/5/90

LIB=resolv
CFLAGS+=-DDEBUG -DLIBC_SCCS -I${.CURDIR}/../libc/include

.PATH:	${.CURDIR}/../libc/net

SRCS=gethnamaddr.c res_mkquery.c res_query.c res_send.c

.include <bsd.lib.mk>
