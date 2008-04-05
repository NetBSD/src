# $NetBSD: Makefile,v 1.5 2008/04/05 19:03:57 xtraeme Exp $

S=		${.CURDIR}/../../../..
.PATH:		$S/dev/isa

KMOD= 		smsc
SRCS= 		real-smsc.c lkm_isa.c
BUILDSYMLINKS+=	smsc.c real-smsc.c

LKM_ISA_IO_PORT=	0x02e
LKM_ISA_DRVNAME=	smsc

.include <bsd.kmod.mk>
