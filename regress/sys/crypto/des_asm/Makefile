#	$NetBSD: Makefile,v 1.4 2016/08/14 19:05:24 joerg Exp $
#	$KAME: Makefile,v 1.5 2000/11/08 05:58:25 itojun Exp $

NOMAN=		# defined

PROG=		destest
SRCS=		destest.c des_ecb.c des_enc.S des_setkey.c

NOPIE=		yes

.include "../Makefile.inc"
.include <bsd.own.mk>

CPPFLAGS+=	-I${SYSDIR} -I${SYSDIR}/arch
.PATH:		${.CURDIR}/../des ${SYSDIR}/crypto/des \
		${SYSDIR}/crypto/des/arch/${MACHINE_ARCH}

.include <bsd.prog.mk>
