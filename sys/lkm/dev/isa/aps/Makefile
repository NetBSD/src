# $NetBSD: Makefile,v 1.2.12.2 2007/11/06 23:33:01 matt Exp $

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
