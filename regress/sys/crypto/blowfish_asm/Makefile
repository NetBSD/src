#	$NetBSD: Makefile,v 1.3 2002/09/18 05:41:51 lukem Exp $
#	$KAME: Makefile,v 1.5 2000/11/08 05:58:24 itojun Exp $

NOMAN=		# defined

PROG=		bftest
SRCS=		bftest.c bf_enc.S bf_skey.c

.include "../Makefile.inc"
.include <bsd.own.mk>

CPPFLAGS+=	-I${SYSDIR} -I${SYSDIR}/arch
.PATH:		${.CURDIR}/../blowfish ${SYSDIR}/crypto/blowfish \
		${SYSDIR}/crypto/blowfish/arch/${MACHINE_ARCH}

.include <bsd.prog.mk>
