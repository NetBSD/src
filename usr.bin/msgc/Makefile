#	$NetBSD: Makefile,v 1.2 1997/10/11 11:39:53 mycroft Exp $

PROG=	msgc
SRCS=	msgmain.c msgparse.y msgscan.l msgdb.c util.c avl.c
.PATH: ${.CURDIR}/../menuc
CFLAGS+=-I. -I${.CURDIR}
WARNS=	1
LDADD+=	-ll
DPADD+=	${LIBL}

CLEANFILES+= y.tab.h

FILES= msg_sys.def
FILESDIR= /usr/share/misc

.include <bsd.prog.mk>
