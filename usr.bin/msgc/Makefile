#	$NetBSD: Makefile,v 1.6 1998/09/27 17:22:12 lukem Exp $

PROG=	msgc
SRCS=	msgmain.c msgparse.y msgscan.l msgdb.c util.c avl.c
.PATH: ${.CURDIR}/../menuc
CPPFLAGS+=-I. -I${.CURDIR}
YHEADER=1
LDADD+=	-ll
DPADD+=	${LIBL}

.if !defined(NOSHARE)
FILES= msg_sys.def
FILESDIR= /usr/share/misc
.endif

.include <bsd.prog.mk>
