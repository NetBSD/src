# $NetBSD: Makefile,v 1.1 2007/09/09 05:27:10 xtraeme Exp $

S=		${.CURDIR}/../../../..
.PATH:		$S/dev/isa

KMOD= 		nsclpcsio
SRCS= 		nsclpcsio_isa.c lkm_isa.c
NOMAN=		# yes

LKM_ISA_IO_PORT=	0	# Driver tries to guess the I/O port
LKM_ISA_DRVNAME=	nsclpcsio
LKM_ISA_DRVNAME_CA=	nsclpcsio_isa

#CPPFLAGS+=	-DMULTIPROCESSOR -DDEBUG -DDIAGNOSTIC -DLOCKDEBUG
CLEANFILES+=	*~ ${KMOD}
WARNS=		2

.include <bsd.kmod.mk>
