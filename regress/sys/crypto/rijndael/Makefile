#	$NetBSD: Makefile,v 1.6 2002/09/18 05:41:52 lukem Exp $
#	$KAME: Makefile,v 1.4 2000/11/08 05:58:26 itojun Exp $

NOMAN=		# defined

PROG=		rijndaeltest
SRCS=		rijndaeltest.c rijndael-alg-fst.c rijndael-api-fst.c

.include "../Makefile.inc"
.include <bsd.own.mk>

CPPFLAGS+=	-I${SYSDIR} "-Dpanic(x)=abort()"
.PATH:		${SYSDIR}/crypto/rijndael

BENCHROUNDS=	100

.include <bsd.prog.mk>
