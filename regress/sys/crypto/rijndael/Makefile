#	$NetBSD: Makefile,v 1.1.1.1 2000/11/01 15:33:26 itojun Exp $
#	$KAME: Makefile,v 1.2 2000/11/01 15:28:11 itojun Exp $

.include "../Makefile.inc"
.include <bsd.own.mk>

WARNS?=	0
PROG=	rijndaeltest
SRCS=	rijndaeltest.c rijndael-alg-fst.c rijndael-api-fst.c
MKMAN=	no

CPPFLAGS+=	-I${SYSDIR} "-Dpanic(x)=abort()"

.PATH:	${SYSDIR}/crypto/rijndael

regress:
	@${.OBJDIR}/${PROG}

.include <bsd.prog.mk>
