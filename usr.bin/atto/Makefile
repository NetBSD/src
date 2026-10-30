#	$NetBSD: Makefile,v 1.2 2026/10/30 16:14:35 nia Exp $

.include <bsd.own.mk>

PROG=	atto
SRCS=	command.c display.c gap.c key.c search.c buffer.c replace.c
SRCS+=	window.c complete.c hilite.c main.c

LDADD+=	-lcurses -lterminfo
DPADD+=	${LIBCURSES} ${LIBTERMINFO}

.ifndef SMALLPROG
SRCS+=	menu.c
LDADD+=	-lmenu
DPADD+=	${LIBMENU}
.else
CPPFLAGS+=	-DSMALL
.endif

WARNS=	3

COPTS.replace.c+=	${CC_WNO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
