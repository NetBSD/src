#	$NetBSD: Makefile,v 1.3 1997/10/19 04:23:03 lukem Exp $

PROG=	menuc
SRCS=	main.c parse.y scan.l avl.c mdb.c util.c
CFLAGS+=-I. -I${.CURDIR}
LDADD+=	-ll
DPADD+=	${LIBL}

CLEANFILES+= y.tab.h

FILES= menu_sys.def
FILESDIR= /usr/share/misc

.include <bsd.prog.mk>
