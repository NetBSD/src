#	$NetBSD: Makefile,v 1.8 2004/03/27 11:34:11 jdolecek Exp $

.include <bsd.own.mk>

DIST=	${NETBSDSRCDIR}/dist
.PATH:	${DIST}/nawk

PROG=	awk
SRCS=	awkgram.y b.c lex.c lib.c main.c parse.c proctab.c run.c tran.c
CPPFLAGS+=	-I${DIST}/nawk -I. -DHAS_ISBLANK
LDADD+=	-lm
DPADD+=	${LIBM}
YHEADER=	yes

# info file originally from GNU awk 3.1.3, adjusted for nawk slightly
.PATH:	${NETBSDSRCDIR}/gnu/dist/gawk
TEXINFO=        awk.info

.include <bsd.info.mk>
.include <bsd.prog.mk>
