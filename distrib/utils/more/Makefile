#	$NetBSD: Makefile,v 1.11 2001/10/05 01:17:28 jmc Exp $
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
DBG= -Os
.endif

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/more.help \
	    ${DESTDIR}/usr/share/misc

.include <bsd.prog.mk>
