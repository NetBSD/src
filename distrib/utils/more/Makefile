#	$NetBSD: Makefile,v 1.13 2002/02/09 13:49:28 lukem Exp $
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
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m ${NONBINMODE} \
		${.CURDIR}/more.help ${DESTDIR}/usr/share/misc

.include <bsd.prog.mk>
