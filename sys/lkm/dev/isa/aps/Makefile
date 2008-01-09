# $NetBSD: Makefile,v 1.2.12.3 2008/01/09 01:56:50 matt Exp $

S=		${.CURDIR}/../../../..
.PATH:		$S/dev/isa $S/dev/ic

KMOD= 		aps
SRCS= 		real-aps.c lkm_isa.c
BUILDSYMLINKS+=	aps.c real-aps.c
NOMAN=		# yes

LKM_ISA_IO_PORT=	0x1600
LKM_ISA_DRVNAME=	aps

CLEANFILES+=	*~ ${KMOD}
WARNS=		2

.include <bsd.kmod.mk>
