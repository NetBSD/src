#	$NetBSD: Makefile,v 1.6 2002/09/18 05:41:52 lukem Exp $
#	$KAME: Makefile,v 1.5 2000/11/08 05:58:25 itojun Exp $

NOMAN=		# defined

PROG=		destest
SRCS=		destest.c des_ecb.c des_enc.c des_setkey.c

.include "../Makefile.inc"
.include <bsd.own.mk>

CPPFLAGS+=	-I${SYSDIR}
.PATH:		${SYSDIR}/crypto/des

.include <bsd.prog.mk>
