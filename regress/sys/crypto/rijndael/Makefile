#	$NetBSD: Makefile,v 1.2 2000/11/08 05:14:57 itojun Exp $
#	$KAME: Makefile,v 1.2 2000/11/01 15:28:11 itojun Exp $

.include "../Makefile.inc"
.include <bsd.own.mk>

PROG=	rijndaeltest
SRCS=	rijndaeltest.c rijndael-alg-fst.c rijndael-api-fst.c
MKMAN=	no

CPPFLAGS+=	-I${SYSDIR} "-Dpanic(x)=abort()"

.PATH:	${SYSDIR}/crypto/rijndael

regress:
	@${.OBJDIR}/${PROG}

.include <bsd.prog.mk>
