#	$NetBSD: Makefile,v 1.5 1998/04/09 00:32:37 tv Exp $

PROG=	msgc
SRCS=	msgmain.c msgparse.y msgscan.l msgdb.c util.c avl.c
.PATH: ${.CURDIR}/../menuc
CPPFLAGS+=-I. -I${.CURDIR}
YHEADER=1
LDADD+=	-ll
DPADD+=	${LIBL}

FILES= msg_sys.def
FILESDIR= /usr/share/misc

.include <bsd.prog.mk>
