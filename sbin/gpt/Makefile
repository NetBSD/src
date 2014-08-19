# $NetBSD: Makefile,v 1.4.12.1 2014/08/20 00:02:25 tls Exp $
# $FreeBSD: src/sbin/gpt/Makefile,v 1.7 2005/09/01 02:49:20 marcel Exp $

PROG=	gpt
SRCS=	add.c backup.c biosboot.c create.c destroy.c gpt.c label.c map.c \
	migrate.c recover.c remove.c resize.c restore.c set.c show.c unset.c
MAN=	gpt.8

LDADD+=	-lprop -lutil
DPADD+= ${LIBPROP} ${LIBUTIL}

.include <bsd.prog.mk>
