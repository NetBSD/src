#	$NetBSD: Makefile,v 1.3.2.1 1997/11/08 10:37:41 lukem Exp $

PROG=	msgc
SRCS=	msgmain.c msgparse.y msgscan.l msgdb.c util.c avl.c
.PATH: ${.CURDIR}/../menuc
CPPFLAGS+=-I. -I${.CURDIR}
LDADD+=	-ll
DPADD+=	${LIBL}

CLEANFILES+= y.tab.h

FILES= msg_sys.def
FILESDIR= /usr/share/misc

.include <bsd.prog.mk>
