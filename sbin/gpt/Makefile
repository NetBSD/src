# $NetBSD: Makefile,v 1.2 2006/10/15 22:36:29 christos Exp $
# $FreeBSD: src/sbin/gpt/Makefile,v 1.7 2005/09/01 02:49:20 marcel Exp $

PROG=	gpt
SRCS=	add.c create.c destroy.c gpt.c label.c map.c migrate.c recover.c \
	remove.c show.c
WARNS?=	4
MAN=	gpt.8

LDADD+=	-lprop -lutil
DPADD+= ${LIBPROP} ${LIBUTIL}

.include <bsd.prog.mk>
