#	from: @(#)Makefile	5.23 (Berkeley) 6/24/90
#	$Id: Makefile,v 1.2 1993/07/30 22:11:51 mycroft Exp $

PROG=	nslookup
SRCS=	main.c getinfo.c debug.c send.c skip.c list.c subr.c
OBJS+=	commands.o
MAN8=	nslookup.0
LFLAGS=	-I
CFLAGS+=-I${.CURDIR}
DPADD=	${LIBLN}
LDADD=	-lln
CLEANFILES+=commands.c lex.yy.c lex.yy.o

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/nslookup.help \
	    ${DESTDIR}/usr/share/misc

.include "../../../Makefile.inc"
.include <bsd.prog.mk>
