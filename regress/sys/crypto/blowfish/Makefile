#	$NetBSD: Makefile,v 1.5 2002/09/18 05:41:51 lukem Exp $
#	$KAME: Makefile,v 1.5 2000/11/08 05:58:24 itojun Exp $

NOMAN=		# defined

PROG=		bftest
SRCS=		bftest.c bf_enc.c bf_skey.c

.include "../Makefile.inc"
.include <bsd.own.mk>

CPPFLAGS+=	-I${SYSDIR}
.PATH:		${SYSDIR}/crypto/blowfish

.include <bsd.prog.mk>
