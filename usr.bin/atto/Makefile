#	$NetBSD: Makefile,v 1.1 2026/10/26 07:52:39 nia Exp $

.include <bsd.own.mk>

PROG=	atto
SRCS=	command.c display.c gap.c key.c search.c buffer.c replace.c
SRCS+=	window.c complete.c hilite.c main.c

LDADD+=	-lcurses -lterminfo
DPADD+=	${LIBCURSES} ${LIBTERMINFO}

WARNS=	3

COPTS.replace.c+=	${CC_WNO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
