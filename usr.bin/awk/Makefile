#	$NetBSD: Makefile,v 1.2 2001/01/23 22:24:20 jdolecek Exp $

DIST=	${.CURDIR}/../../dist
.PATH:	${DIST}/nawk

PROG=	awk
SRCS=	awkgram.y b.c lex.c lib.c main.c parse.c proctab.c run.c tran.c
CPPFLAGS+=	-I${DIST}/nawk -I.
LDADD+=	-lm
DPADD+=	${LIBM}
YHEADER=	yes

.include <bsd.prog.mk>
