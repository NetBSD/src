#	from: @(#)Makefile	5.1 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.3 1993/08/01 05:46:14 mycroft Exp $

PROG=	teachgammon
CFLAGS+=-DV7 -I${.CURDIR}/../common_source
SRCS=	allow.c board.c check.c data.c fancy.c init.c odds.c one.c save.c \
	subs.c table.c teach.c ttext1.c ttext2.c tutor.c
DPADD=	${LIBTERM}
LDADD=	-ltermcap
HIDEGAME=hidegame
NOMAN=	noman

.PATH:	${.CURDIR}/../common_source

.include "../../Makefile.inc"
.include <bsd.prog.mk>
