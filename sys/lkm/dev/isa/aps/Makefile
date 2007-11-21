# $NetBSD: Makefile,v 1.2.6.1 2007/11/21 21:19:48 bouyer Exp $

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
