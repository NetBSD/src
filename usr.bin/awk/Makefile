#	$NetBSD: Makefile,v 1.1 2001/01/23 16:25:22 jdolecek Exp $

DIST=	${.CURDIR}/../../dist
.PATH:	${DIST}/nawk

PROG=	awk
SRCS=	awkgram.y b.c lex.c lib.c main.c parse.c proctab.c run.c tran.c
CPPFLAGS+=	-I${DIST}/nawk -I.
LDADD+=	-lm
DPADD+=	${LIBM}
YFLAGS=	-d

CLEANFILES+=	y.tab.h

.include <bsd.prog.mk>
