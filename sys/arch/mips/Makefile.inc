#	$NetBSD: Makefile.inc,v 1.5 2007/02/21 02:28:30 dyoung Exp $

AMIPS=	${SYSDIR}/arch/mips/mips/*.S
SMIPS=	${SYSDIR}/arch/mips/mips/*.[ch] ${SYSDIR}/arch/mips/include/*.h
SMIPS+=	${SYSDIR}/arch/mips/alchemy/*.[ch]
SMIPS+=	${SYSDIR}/arch/mips/alchemy/dev/*.[ch]
SMIPS+=	${SYSDIR}/arch/mips/alchemy/include/*.h
SMIPS+=	${SYSDIR}/arch/mips/atheros/*.[ch]
SMIPS+=	${SYSDIR}/arch/mips/atheros/dev/*.[ch]
SMIPS+=	${SYSDIR}/arch/mips/atheros/include/*.h
SMIPS+=	${SYSDIR}/arch/mips/bonito/*.[ch]
