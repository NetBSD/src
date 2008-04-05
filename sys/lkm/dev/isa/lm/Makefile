# $NetBSD: Makefile,v 1.3 2008/04/05 19:03:57 xtraeme Exp $

S=		${.CURDIR}/../../../..
.PATH:		$S/dev/isa $S/dev/ic

KMOD= 		lm
SRCS= 		lm_isa.c nslm7x.c lkm_isa.c

LKM_ISA_IO_PORT=	0x290 # Change this to another port if desired
LKM_ISA_DRVNAME=	lm
LKM_ISA_DRVNAME_CA=	lm_isa

.include <bsd.kmod.mk>
