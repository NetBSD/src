#	from: @(#)Makefile	5.1 (Berkeley) 6/5/90
#	$Id: Makefile,v 1.4 1997/07/23 03:14:11 mrg Exp $

LIB=resolv
CFLAGS+=-DDEBUG -DLIBC_SCCS -I${.CURDIR}/../libc/include

.PATH:	${.CURDIR}/../libc/net

SRCS=gethnamaddr.c res_mkquery.c res_query.c res_send.c

.include <bsd.lib.mk>
