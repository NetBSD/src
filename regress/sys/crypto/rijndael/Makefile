#	$NetBSD: Makefile,v 1.3 2000/11/08 06:02:02 itojun Exp $
#	$KAME: Makefile,v 1.4 2000/11/08 05:58:26 itojun Exp $

.include "../Makefile.inc"
.include <bsd.own.mk>

PROG=	rijndaeltest
SRCS=	rijndaeltest.c rijndael-alg-fst.c rijndael-api-fst.c
MKMAN=	no

CPPFLAGS+=	-I${SYSDIR} "-Dpanic(x)=abort()"

.PATH:	${SYSDIR}/crypto/rijndael

BENCHROUNDS=	100

.include <bsd.prog.mk>
