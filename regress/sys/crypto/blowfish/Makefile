#	$NetBSD: Makefile,v 1.4 2001/12/12 01:24:14 tv Exp $
#	$KAME: Makefile,v 1.5 2000/11/08 05:58:24 itojun Exp $

PROG=		bftest
SRCS=		bftest.c bf_enc.c bf_skey.c
NOMAN=		# defined

.include "../Makefile.inc"
.include <bsd.own.mk>

CPPFLAGS+=	-I${SYSDIR}
.PATH:		${SYSDIR}/crypto/blowfish

.include <bsd.prog.mk>
