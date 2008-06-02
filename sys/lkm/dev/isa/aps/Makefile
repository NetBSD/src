# $NetBSD: Makefile,v 1.3.14.1 2008/06/02 13:24:17 mjf Exp $

S=		${.CURDIR}/../../../..
.PATH:		$S/dev/isa $S/dev/ic

KMOD= 		aps
SRCS= 		real-aps.c lkm_isa.c
BUILDSYMLINKS+=	aps.c real-aps.c

LKM_ISA_IO_PORT=	0x1600
LKM_ISA_DRVNAME=	aps

.include <bsd.kmod.mk>
