#	from: @(#)Makefile	5.23 (Berkeley) 6/24/90
#	$Id: Makefile,v 1.4 1994/12/22 11:37:21 cgd Exp $

PROG=	nslookup
SRCS=	main.c getinfo.c debug.c send.c skip.c list.c subr.c
OBJS+=	commands.o
MAN=	nslookup.8
LFLAGS=	-I
CFLAGS+=-I${.CURDIR}
LDADD=	-ll
DPADD=	${LIBL}
CLEANFILES+=commands.c lex.yy.c lex.yy.o

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/nslookup.help \
	    ${DESTDIR}/usr/share/misc

.include "../../../Makefile.inc"
.include <bsd.prog.mk>
