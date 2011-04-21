#	$NetBSD: Makefile.inc,v 1.6.60.1 2011/04/21 01:41:10 rmind Exp $

AMIPS=	${SYSDIR}/arch/mips/mips/*.S
AMIPS+=	${SYSDIR}/arch/mips/rmi/*.S
SMIPS=	${SYSDIR}/arch/mips/mips/*.[ch]
SMIPS+=	${SYSDIR}/arch/mips/include/*.h
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
SMIPS+=	${SYSDIR}/arch/mips/cfe/*.[ch]
SMIPS+=	${SYSDIR}/arch/mips/rmi/*.[ch]
SMIPS+=	${SYSDIR}/arch/mips/sibyte/dev/*.[ch]
SMIPS+=	${SYSDIR}/arch/mips/sibyte/include/*.[ch]
SMIPS+=	${SYSDIR}/arch/mips/sibyte/pci/*.[ch]
