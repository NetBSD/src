#	$NetBSD: Makefile,v 1.15 2010/02/03 15:34:38 roy Exp $
#
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

WARNS?=		1
PROG=		more
NOMAN=		# defined

SRCS=		ch.c command.c decode.c help.c input.c line.c linenum.c main.c \
		option.c os.c output.c position.c prim.c screen.c signal.c \
		ttyin.c

CPPFLAGS+=	-I${.CURDIR}
DPADD=		${LIBTERMLIB}
LDADD=		-ltermlib

beforeinstall:
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m ${NONBINMODE} \
		${.CURDIR}/more.help ${DESTDIR}/usr/share/misc

.include <bsd.prog.mk>
