#	$NetBSD: Makefile,v 1.12 2001/12/12 00:05:10 tv Exp $
#
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=		more
NOMAN=		# defined

SRCS=		ch.c command.c decode.c help.c input.c line.c linenum.c main.c \
		option.c os.c output.c position.c prim.c screen.c signal.c \
		ttyin.c

CPPFLAGS+=	-I${.CURDIR}
DPADD=		${LIBTERMCAP}
LDADD=		-ltermcap

beforeinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/more.help \
	    ${DESTDIR}/usr/share/misc

.include <bsd.prog.mk>
