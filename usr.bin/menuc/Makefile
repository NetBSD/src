#	$NetBSD: Makefile,v 1.6 1998/09/27 17:22:12 lukem Exp $

PROG=	menuc
SRCS=	main.c parse.y scan.l avl.c mdb.c util.c
CPPFLAGS+=-I. -I${.CURDIR}
YHEADER=1
LDADD+=	-ll
DPADD+=	${LIBL}

.if !defined(NOSHARE)
FILES= menu_sys.def
FILESDIR= /usr/share/misc
.endif

.include <bsd.prog.mk>
