#	from: @(#)Makefile	5.1 (Berkeley) 6/5/90
#	$Id: Makefile,v 1.3 1997/07/20 22:38:45 christos Exp $

LIB=resolv
CFLAGS+=-DDEBUG -DLIBC_SCCS

.PATH:	${.CURDIR}/../libc/net

SRCS=gethnamaddr.c res_mkquery.c res_query.c res_send.c

.include <bsd.lib.mk>
