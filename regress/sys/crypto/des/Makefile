#	$NetBSD: Makefile,v 1.5 2001/12/12 01:24:15 tv Exp $
#	$KAME: Makefile,v 1.5 2000/11/08 05:58:25 itojun Exp $

PROG=		destest
SRCS=		destest.c des_ecb.c des_enc.c des_setkey.c
NOMAN=		# defined

.include "../Makefile.inc"
.include <bsd.own.mk>

CPPFLAGS+=	-I${SYSDIR}
.PATH:		${SYSDIR}/crypto/des

.include <bsd.prog.mk>
