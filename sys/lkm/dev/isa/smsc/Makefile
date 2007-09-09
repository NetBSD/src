# $NetBSD: Makefile,v 1.3 2007/09/09 13:52:23 xtraeme Exp $

S=		${.CURDIR}/../../../..
.PATH:		$S/dev/isa

KMOD= 		smsc_isa
SRCS= 		smsc.c lkm_isa.c
NOMAN=		# yes

LKM_ISA_IO_PORT=	0x02e
LKM_ISA_DRVNAME=	smsc

CLEANFILES+=	*~ ${KMOD}
WARNS=		2

.include <bsd.kmod.mk>
