#	$NetBSD: Makefile,v 1.1.1.1 2000/11/01 15:33:25 itojun Exp $
#	$KAME: Makefile,v 1.2 2000/11/01 15:28:10 itojun Exp $

.include "../Makefile.inc"
.include <bsd.own.mk>

WARNS?=	1
PROG=	casttest
SRCS=	casttest.c cast128.c
MKMAN=	no

CPPFLAGS+=	-I${SYSDIR}

.PATH:	${SYSDIR}/crypto/cast128

regress:
	@${.OBJDIR}/${PROG}

.include <bsd.prog.mk>
