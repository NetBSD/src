#	$NetBSD: Makefile,v 1.7.2.1 1997/11/08 15:12:07 lukem Exp $
# from: $Id: Makefile,v 8.1 1994/12/15 06:23:45 vixie Exp 

.PATH:	${.CURDIR}/../nslookup \
	${.CURDIR}/../man \
	${.CURDIR}/../../../lib/libc/net

PROG=	dig
SRCS=	dig.c list.c subr.c debug.c send.c  gethnamaddr.c res_init.c \
	res_mkquery.c res_query.c res_send.c
CPPFLAGS+=	-I${.CURDIR}/.. -I${.CURDIR}/../../../lib/libc/include -DDEBUG

.include "../../Makefile.inc"
.include <bsd.prog.mk>
