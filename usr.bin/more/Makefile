#	from: @(#)Makefile	5.6 (Berkeley) 3/12/91
#	$Id: Makefile,v 1.5 1993/11/11 01:30:28 jtc Exp $

PROG=	more
CFLAGS+=-I${.CURDIR} -DREGEX 
SRCS=	ch.c command.c decode.c filename.c help.c input.c line.c \
	linenum.c main.c option.c os.c output.c position.c prim.c \
	screen.c signal.c tags.c ttyin.c 
LDADD+=	-ltermcap
DPADD+=	${LIBTERM}
MLINKS=	more.1 page.1
LINKS=	${BINDIR}/more ${BINDIR}/page

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/more.help \
	    ${DESTDIR}/usr/share/misc

.include <bsd.prog.mk>
