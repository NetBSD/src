#	$NetBSD: Makefile,v 1.7 1999/02/13 02:54:56 lukem Exp $

.include <bsd.own.mk>

PROG=	msgc
SRCS=	msgmain.c msgparse.y msgscan.l msgdb.c util.c avl.c
.PATH: ${.CURDIR}/../menuc
CPPFLAGS+=-I. -I${.CURDIR}
YHEADER=1
LDADD+=	-ll
DPADD+=	${LIBL}

.if ${MKSHARE} != "no"
FILES= msg_sys.def
FILESDIR= /usr/share/misc
.endif

.include <bsd.prog.mk>
