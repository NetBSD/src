#	$NetBSD: Makefile,v 1.3.2.1 1997/11/08 10:37:34 lukem Exp $

PROG=	menuc
SRCS=	main.c parse.y scan.l avl.c mdb.c util.c
CPPFLAGS+=-I. -I${.CURDIR}
LDADD+=	-ll
DPADD+=	${LIBL}

CLEANFILES+= y.tab.h

FILES= menu_sys.def
FILESDIR= /usr/share/misc

.include <bsd.prog.mk>
