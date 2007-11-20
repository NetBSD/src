# $NetBSD: Makefile,v 1.3 2007/11/20 21:47:34 xtraeme Exp $

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
