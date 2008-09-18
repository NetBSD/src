#	$NetBSD: Makefile,v 1.9.22.1 2008/09/18 04:29:07 wrstuden Exp $

.include <bsd.own.mk>

DIST=	${NETBSDSRCDIR}/dist
.PATH:	${DIST}/nawk

PROG=	awk
SRCS=	awkgram.y b.c lex.c lib.c main.c parse.c proctab.c run.c tran.c
CPPFLAGS+=	-I${DIST}/nawk -I. -DHAS_ISBLANK
LDADD+=	-lm
DPADD+=	${LIBM}
YHEADER=	yes
.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS=	-Wno-pointer-sign
.endif

# info file originally from GNU awk 3.1.3, adjusted for nawk slightly
.PATH:	${NETBSDSRCDIR}/gnu/dist/gawk
TEXINFO=        awk.info

.include <bsd.info.mk>
.include <bsd.prog.mk>
