#	$NetBSD: Makefile,v 1.5 1998/04/09 00:32:37 tv Exp $

PROG=	menuc
SRCS=	main.c parse.y scan.l avl.c mdb.c util.c
CPPFLAGS+=-I. -I${.CURDIR}
YHEADER=1
LDADD+=	-ll
DPADD+=	${LIBL}

FILES= menu_sys.def
FILESDIR= /usr/share/misc

.include <bsd.prog.mk>
