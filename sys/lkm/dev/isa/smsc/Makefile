# $NetBSD: Makefile,v 1.4.14.1 2008/06/02 13:24:17 mjf Exp $

S=		${.CURDIR}/../../../..
.PATH:		$S/dev/isa

KMOD= 		smsc
SRCS= 		real-smsc.c lkm_isa.c
BUILDSYMLINKS+=	smsc.c real-smsc.c

LKM_ISA_IO_PORT=	0x02e
LKM_ISA_DRVNAME=	smsc

.include <bsd.kmod.mk>
