#	$NetBSD: Makefile,v 1.2 2002/02/27 02:21:36 itojun Exp $
#	$KAME: Makefile,v 1.5 2000/11/08 05:58:24 itojun Exp $

PROG=		bftest
SRCS=		bftest.c bf_enc.S bf_skey.c
NOMAN=		# defined

.include "../Makefile.inc"
.include <bsd.own.mk>

CPPFLAGS+=	-I${SYSDIR} -I${SYSDIR}/arch
.PATH:		${.CURDIR}/../blowfish ${SYSDIR}/crypto/blowfish \
		${SYSDIR}/crypto/blowfish/arch/${MACHINE_ARCH}

.include <bsd.prog.mk>
