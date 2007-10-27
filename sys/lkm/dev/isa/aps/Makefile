# $NetBSD: Makefile,v 1.2.8.2 2007/10/27 11:35:46 yamt Exp $

S=		${.CURDIR}/../../../..
.PATH:		$S/dev/isa $S/dev/ic

KMOD= 		aps_isa
SRCS= 		aps.c lkm_isa.c
NOMAN=		# yes

LKM_ISA_IO_PORT=	0x1600
LKM_ISA_DRVNAME=	aps

CLEANFILES+=	*~ ${KMOD}
WARNS=		2

.include <bsd.kmod.mk>
