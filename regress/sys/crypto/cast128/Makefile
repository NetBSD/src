#	$NetBSD: Makefile,v 1.2 2000/11/08 05:14:57 itojun Exp $
#	$KAME: Makefile,v 1.2 2000/11/01 15:28:10 itojun Exp $

.include "../Makefile.inc"
.include <bsd.own.mk>

PROG=	casttest
SRCS=	casttest.c cast128.c
MKMAN=	no

CPPFLAGS+=	-I${SYSDIR}

.PATH:	${SYSDIR}/crypto/cast128

regress:
	@${.OBJDIR}/${PROG}

.include <bsd.prog.mk>
