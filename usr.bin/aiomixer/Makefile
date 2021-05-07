# $NetBSD: Makefile,v 1.1 2021/05/07 16:29:24 nia Exp $

PROG=	aiomixer
SRCS+=	main.c draw.c parse.c

LDADD+=	-lcurses
DPADD+=	$(LIBCURSES)

WARNS= 6

.include <bsd.prog.mk>
