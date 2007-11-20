# $NetBSD: Makefile,v 1.4 2007/11/20 21:47:34 xtraeme Exp $

S=		${.CURDIR}/../../../..
.PATH:		$S/dev/isa

KMOD= 		smsc
SRCS= 		real-smsc.c lkm_isa.c
BUILDSYMLINKS+=	smsc.c real-smsc.c
NOMAN=		# yes

LKM_ISA_IO_PORT=	0x02e
LKM_ISA_DRVNAME=	smsc

CLEANFILES+=	*~ ${KMOD}
WARNS=		2

.include <bsd.kmod.mk>
