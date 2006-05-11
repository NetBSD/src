#	$NetBSD: Makefile,v 1.9 2006/05/11 01:19:55 mrg Exp $

.include <bsd.own.mk>

DIST=	${NETBSDSRCDIR}/dist
.PATH:	${DIST}/nawk

PROG=	awk
SRCS=	awkgram.y b.c lex.c lib.c main.c parse.c proctab.c run.c tran.c
CPPFLAGS+=	-I${DIST}/nawk -I. -DHAS_ISBLANK
LDADD+=	-lm
DPADD+=	${LIBM}
YHEADER=	yes
.if ${HAVE_GCC} == 4
COPTS=	-Wno-pointer-sign
.endif

# info file originally from GNU awk 3.1.3, adjusted for nawk slightly
.PATH:	${NETBSDSRCDIR}/gnu/dist/gawk
TEXINFO=        awk.info

.include <bsd.info.mk>
.include <bsd.prog.mk>
