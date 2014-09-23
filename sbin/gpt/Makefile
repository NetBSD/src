# $NetBSD: Makefile,v 1.9 2014/09/23 07:47:54 jnemeth Exp $
# $FreeBSD: src/sbin/gpt/Makefile,v 1.7 2005/09/01 02:49:20 marcel Exp $

PROG=	gpt
SRCS=	add.c backup.c biosboot.c create.c destroy.c gpt.c label.c map.c \
	migrate.c recover.c remove.c resize.c resizedisk.c restore.c \
	set.c show.c unset.c
MAN=	gpt.8

LDADD+=	-lprop -lutil
DPADD+= ${LIBPROP} ${LIBUTIL}

.include <bsd.prog.mk>
