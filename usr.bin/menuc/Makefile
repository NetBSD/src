#	$NetBSD: Makefile,v 1.4 1997/10/22 01:53:41 lukem Exp $

PROG=	menuc
SRCS=	main.c parse.y scan.l avl.c mdb.c util.c
CPPFLAGS+=-I. -I${.CURDIR}
LDADD+=	-ll
DPADD+=	${LIBL}

CLEANFILES+= y.tab.h

FILES= menu_sys.def
FILESDIR= /usr/share/misc

.include <bsd.prog.mk>
