# $NetBSD: Makefile,v 1.2.28.1 2009/05/13 19:19:02 jym Exp $
# $FreeBSD: src/sbin/gpt/Makefile,v 1.7 2005/09/01 02:49:20 marcel Exp $

PROG=	gpt
SRCS=	add.c create.c destroy.c gpt.c label.c map.c migrate.c recover.c \
	remove.c show.c
MAN=	gpt.8

LDADD+=	-lprop -lutil
DPADD+= ${LIBPROP} ${LIBUTIL}

.include <bsd.prog.mk>
