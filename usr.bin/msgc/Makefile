#	$NetBSD: Makefile,v 1.4 1997/10/22 01:47:23 lukem Exp $

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
