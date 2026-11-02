#	$NetBSD: Makefile,v 1.4 2026/11/02 08:33:37 nia Exp $

.include <bsd.own.mk>

USE_FORT?= yes	# data-driven bugs?

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

WARNS=	5

COPTS.replace.c+=	${CC_WNO_FORMAT_TRUNCATION}

MLINKS+=	atto.1 natto.1
LINKS+=		${BINDIR}/atto ${BINDIR}/natto

.include <bsd.prog.mk>
