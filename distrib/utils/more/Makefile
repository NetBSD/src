#	$NetBSD: Makefile,v 1.4 1998/01/09 08:03:23 perry Exp $
#
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	more
NOMAN=1

CFLAGS+=-I${.CURDIR}
SRCS=	ch.c command.c decode.c help.c input.c line.c linenum.c main.c \
	option.c os.c output.c position.c prim.c screen.c signal.c \
	ttyin.c
DPADD=	${LIBTERM} ${LIBCOMPAT}
LDADD=	-ltermcap -lcompat

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/more.help \
	    ${DESTDIR}/usr/share/misc

.include <bsd.prog.mk>
