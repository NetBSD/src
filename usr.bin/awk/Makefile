#	$NetBSD: Makefile,v 1.5 2002/09/18 14:00:34 lukem Exp $

.include <bsd.own.mk>

DIST=	${NETBSDSRCDIR}/dist
.PATH:	${DIST}/nawk

PROG=	awk
SRCS=	awkgram.y b.c lex.c lib.c main.c parse.c proctab.c run.c tran.c
CPPFLAGS+=	-I${DIST}/nawk -I.
LDADD+=	-lm
DPADD+=	${LIBM}
YHEADER=	yes

.include <bsd.prog.mk>
