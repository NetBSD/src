#	$NetBSD: Makefile,v 1.2 2000/11/08 05:14:57 itojun Exp $
#	$KAME: Makefile,v 1.3 2000/11/01 15:28:10 itojun Exp $

.include "../Makefile.inc"
.include <bsd.own.mk>

PROG=	destest
SRCS=	destest.c des_ecb.c des_setkey.c
MKMAN=	no

CPPFLAGS+=	-I${SYSDIR}

.PATH:	${SYSDIR}/crypto/des

regress:
	@${.OBJDIR}/${PROG}

.include <bsd.prog.mk>
