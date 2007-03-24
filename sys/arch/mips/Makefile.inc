#	$NetBSD: Makefile.inc,v 1.3.10.2 2007/03/24 14:54:49 yamt Exp $

AMIPS=	${SYSDIR}/arch/mips/mips/*.S
SMIPS=	${SYSDIR}/arch/mips/mips/*.[ch] ${SYSDIR}/arch/mips/include/*.h
SMIPS+=	${SYSDIR}/arch/mips/adm5120/*.[ch]
SMIPS+=	${SYSDIR}/arch/mips/adm5120/dev/*.[ch]
SMIPS+=	${SYSDIR}/arch/mips/adm5120/include/*.h
SMIPS+=	${SYSDIR}/arch/mips/alchemy/*.[ch]
SMIPS+=	${SYSDIR}/arch/mips/alchemy/dev/*.[ch]
SMIPS+=	${SYSDIR}/arch/mips/alchemy/include/*.h
SMIPS+=	${SYSDIR}/arch/mips/atheros/*.[ch]
SMIPS+=	${SYSDIR}/arch/mips/atheros/dev/*.[ch]
SMIPS+=	${SYSDIR}/arch/mips/atheros/include/*.h
SMIPS+=	${SYSDIR}/arch/mips/bonito/*.[ch]
