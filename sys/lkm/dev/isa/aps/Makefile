# $NetBSD: Makefile,v 1.4 2008/04/05 19:03:57 xtraeme Exp $

S=		${.CURDIR}/../../../..
.PATH:		$S/dev/isa $S/dev/ic

KMOD= 		aps
SRCS= 		real-aps.c lkm_isa.c
BUILDSYMLINKS+=	aps.c real-aps.c

LKM_ISA_IO_PORT=	0x1600
LKM_ISA_DRVNAME=	aps

.include <bsd.kmod.mk>
