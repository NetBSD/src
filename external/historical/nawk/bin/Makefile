#	$NetBSD: Makefile,v 1.3 2010/12/12 19:01:03 christos Exp $

WARNS?= 4

.include <bsd.own.mk>

BINDIR?= /usr/bin
DIST=	${.CURDIR}/../dist
.PATH:	${DIST}

PROG=	awk
SRCS=	awkgram.y b.c lex.c lib.c main.c parse.c proctab.c run.c tran.c
CPPFLAGS+=	-I${DIST} -I.
LDADD+=	-lm
.if !defined(HOSTPROG)
DPADD+=	${LIBM}
.endif
YHEADER=	yes
.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS+=	-Wno-pointer-sign
.endif

.include <bsd.prog.mk>
