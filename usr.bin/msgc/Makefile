#	$NetBSD: Makefile,v 1.3 1997/10/19 04:23:05 lukem Exp $

PROG=	msgc
SRCS=	msgmain.c msgparse.y msgscan.l msgdb.c util.c avl.c
.PATH: ${.CURDIR}/../menuc
CFLAGS+=-I. -I${.CURDIR}
LDADD+=	-ll
DPADD+=	${LIBL}

CLEANFILES+= y.tab.h

FILES= msg_sys.def
FILESDIR= /usr/share/misc

.include <bsd.prog.mk>
