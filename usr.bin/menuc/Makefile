#	$NetBSD: Makefile,v 1.7 1999/02/13 02:54:56 lukem Exp $

.include <bsd.own.mk>

PROG=	menuc
SRCS=	main.c parse.y scan.l avl.c mdb.c util.c
CPPFLAGS+=-I. -I${.CURDIR}
YHEADER=1
LDADD+=	-ll
DPADD+=	${LIBL}

.if ${MKSHARE} != "no"
FILES= menu_sys.def
FILESDIR= /usr/share/misc
.endif

.include <bsd.prog.mk>
