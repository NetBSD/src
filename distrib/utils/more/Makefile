#	$NetBSD: Makefile,v 1.10 2001/08/20 12:19:44 martin Exp $
#
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	more
MKMAN=	no

SRCS=	ch.c command.c decode.c help.c input.c line.c linenum.c main.c \
	option.c os.c output.c position.c prim.c screen.c signal.c \
	ttyin.c
DPADD=	${LIBTERMCAP}
LDADD=	-ltermcap
LDSTATIC?=-static
CPPFLAGS+=-I${.CURDIR}
.if ${MACHINE_ARCH} != "sparc64"
COPTS+= -Os
.endif

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/more.help \
	    ${DESTDIR}/usr/share/misc

.include <bsd.prog.mk>
