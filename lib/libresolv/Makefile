#	$NetBSD: Makefile,v 1.5.2.1 1997/11/09 01:00:51 lukem Exp $
#	from: @(#)Makefile	5.1 (Berkeley) 6/5/90

LIB=resolv
CPPFLAGS+=-DDEBUG -DLIBC_SCCS -I${.CURDIR}/../libc/include

.PATH:	${.CURDIR}/../libc/net

SRCS=gethnamaddr.c res_mkquery.c res_query.c res_send.c

.include <bsd.lib.mk>
