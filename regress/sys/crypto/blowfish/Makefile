#	$NetBSD: Makefile,v 1.2 2000/11/08 05:14:56 itojun Exp $
#	$KAME: Makefile,v 1.3 2000/11/01 15:28:10 itojun Exp $

.include "../Makefile.inc"
.include <bsd.own.mk>

PROG=	bftest
SRCS=	bftest.c bf_enc.c bf_skey.c
MKMAN=	no

CPPFLAGS+=	-I${SYSDIR}

.PATH:	${SYSDIR}/crypto/blowfish

regress:
	@${.OBJDIR}/${PROG}

.include <bsd.prog.mk>
