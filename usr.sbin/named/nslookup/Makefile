#	$NetBSD: Makefile,v 1.2 1996/10/18 06:28:49 thorpej Exp $
# from: $Id: Makefile,v 8.1 1994/12/15 06:23:48 vixie Exp 

.PATH:	${.CURDIR}/../nslookup \
	${.CURDIR}/../man

PROG=	nslookup
SRCS=	main.c getinfo.c debug.c send.c skip.c list.c subr.c commands.l
CFLAGS+= ${INCLUDE}
DPADD=	${LIBL}
LDADD=	-ll
CLEANFILES+=	commands.c lex.yy.c lex.yy.o
MAN=	nslookup.8

beforeinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 \
		${.CURDIR}/../nslookup/nslookup.help \
		${DESTDIR}/usr/share/misc/nslookup.help

.include <bsd.prog.mk>
.include "../Makefile.inc"
.include "../../Makefile.inc"
