#	$NetBSD: Makefile,v 1.1.1.1 2000/11/01 15:33:23 itojun Exp $
#	$KAME: Makefile,v 1.3 2000/11/01 15:28:10 itojun Exp $

.include "../Makefile.inc"
.include <bsd.own.mk>

WARNS?=	1
PROG=	destest
SRCS=	destest.c des_ecb.c des_setkey.c
MKMAN=	no

CPPFLAGS+=	-I${SYSDIR}

.PATH:	${SYSDIR}/crypto/des

regress:
	@${.OBJDIR}/${PROG}

.include <bsd.prog.mk>
