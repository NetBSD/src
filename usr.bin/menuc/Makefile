#	$NetBSD: Makefile,v 1.2 1997/10/11 11:37:21 mycroft Exp $

PROG=	menuc
SRCS=	main.c parse.y scan.l avl.c mdb.c util.c
CFLAGS+=-I. -I${.CURDIR}
WARNS=	1
LDADD+=	-ll
DPADD+=	${LIBL}

CLEANFILES+= y.tab.h

FILES= menu_sys.def
FILESDIR= /usr/share/misc

.include <bsd.prog.mk>
