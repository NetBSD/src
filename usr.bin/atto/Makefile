#	$NetBSD: Makefile,v 1.3 2026/10/30 22:32:03 nia Exp $

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

WARNS=	3

COPTS.replace.c+=	${CC_WNO_FORMAT_TRUNCATION}

MLINKS+=	atto.1 natto.1
LINKS+=		${BINDIR}/atto ${BINDIR}/natto

.include <bsd.prog.mk>
