#	$NetBSD: Makefile,v 1.2 2002/02/27 02:21:36 itojun Exp $
#	$KAME: Makefile,v 1.5 2000/11/08 05:58:25 itojun Exp $

PROG=		destest
SRCS=		destest.c des_ecb.c des_enc.S des_setkey.c
NOMAN=		# defined

.include "../Makefile.inc"
.include <bsd.own.mk>

CPPFLAGS+=	-I${SYSDIR} -I${SYSDIR}/arch
.PATH:		${.CURDIR}/../des ${SYSDIR}/crypto/des \
		${SYSDIR}/crypto/des/arch/${MACHINE_ARCH}

.include <bsd.prog.mk>
