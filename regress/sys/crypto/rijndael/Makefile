#	$NetBSD: Makefile,v 1.5 2001/12/12 01:24:15 tv Exp $
#	$KAME: Makefile,v 1.4 2000/11/08 05:58:26 itojun Exp $

PROG=		rijndaeltest
SRCS=		rijndaeltest.c rijndael-alg-fst.c rijndael-api-fst.c
NOMAN=		# defined

.include "../Makefile.inc"
.include <bsd.own.mk>

CPPFLAGS+=	-I${SYSDIR} "-Dpanic(x)=abort()"
.PATH:		${SYSDIR}/crypto/rijndael

BENCHROUNDS=	100

.include <bsd.prog.mk>
