#	$NetBSD: Makefile,v 1.7 1997/05/08 21:11:06 gwr Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	teachgammon
CFLAGS+=-DV7 -I${.CURDIR}/../common_source
SRCS=	allow.c board.c check.c data.c fancy.c init.c odds.c one.c save.c \
	subs.c table.c teach.c ttext1.c ttext2.c tutor.c
DPADD=	${LIBTERM} ${LIBCOMPAT}
LDADD=	-ltermcap -lcompat
HIDEGAME=hidegame
NOMAN=	noman

.PATH:	${.CURDIR}/../common_source

.include "../../Makefile.inc"
.include <bsd.prog.mk>
