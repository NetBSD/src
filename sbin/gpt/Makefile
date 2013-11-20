# $NetBSD: Makefile,v 1.5 2013/11/20 08:08:47 jnemeth Exp $
# $FreeBSD: src/sbin/gpt/Makefile,v 1.7 2005/09/01 02:49:20 marcel Exp $

PROG=	gpt
SRCS=	add.c biosboot.c create.c destroy.c gpt.c label.c map.c migrate.c \
	recover.c remove.c resize.c show.c
MAN=	gpt.8

LDADD+=	-lprop -lutil
DPADD+= ${LIBPROP} ${LIBUTIL}

.include <bsd.prog.mk>
