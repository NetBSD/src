#	$NetBSD: Makefile,v 1.8 2001/10/05 22:54:51 jmc Exp $

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

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
