#	from: @(#)Makefile	5.1 (Berkeley) 6/5/90
#	$Id: Makefile,v 1.2 1993/08/01 05:40:18 mycroft Exp $

LIB=resolv
CFLAGS+=-DDEBUG -DLIBC_SCCS

.PATH:	${.CURDIR}/../libc/net

SRCS=gethostnamadr.c res_mkquery.c res_query.c res_send.c

.include <bsd.lib.mk>
