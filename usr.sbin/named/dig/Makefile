#	$NetBSD: Makefile,v 1.4 1997/05/06 20:46:27 gwr Exp $
# from: $Id: Makefile,v 8.1 1994/12/15 06:23:45 vixie Exp 

.PATH.c: ${.CURDIR}/../nslookup \
	${.CURDIR}/../man \
	${.CURDIR}/../../../lib/libc/net

PROG=	dig
SRCS=	dig.c list.c subr.c debug.c send.c  gethostnamadr.c res_init.c \
	res_mkquery.c res_query.c res_send.c
CFLAGS+=	-I${.CURDIR}/.. -DDEBUG

.include "../../Makefile.inc"
.include <bsd.prog.mk>
