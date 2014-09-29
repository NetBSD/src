# $NetBSD: Makefile,v 1.11 2014/09/29 21:04:34 christos Exp $
# $FreeBSD: src/sbin/gpt/Makefile,v 1.7 2005/09/01 02:49:20 marcel Exp $

PROG=	gpt
SRCS=	add.c biosboot.c create.c destroy.c gpt.c label.c map.c \
	migrate.c recover.c remove.c resize.c resizedisk.c \
	set.c show.c type.c unset.c
MAN=	gpt.8

.if (${HOSTPROG:U} == "")
SRCS+= backup.c restore.c
LDADD+=	-lprop -lutil
DPADD+= ${LIBPROP} ${LIBUTIL}
.endif

.include <bsd.prog.mk>
