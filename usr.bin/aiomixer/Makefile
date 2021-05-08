# $NetBSD: Makefile,v 1.2 2021/05/08 14:11:37 cjep Exp $

PROG=	aiomixer
SRCS+=	main.c draw.c parse.c

LDADD+=	-lcurses -lterminfo
DPADD+=	${LIBCURSES} ${LIBTERMINFO}

WARNS= 6

.include <bsd.prog.mk>
