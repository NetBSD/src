# $NetBSD: Makefile,v 1.1 2007/09/09 05:11:01 xtraeme Exp $

S=		${.CURDIR}/../../../..
.PATH:		$S/dev/isa

KMOD= 		it_isa
SRCS+= 		lkm_isa.c it.c
NOMAN=		# yes

LKM_ISA_IO_PORT=	0x290	# Other ports: 0xc00, 0xd00
LKM_ISA_DRVNAME=	it
LKM_ISA_DRVNAME_CA=	it_isa

#CPPFLAGS+=	-DMULTIPROCESSOR -DDEBUG -DDIAGNOSTIC -DLOCKDEBUG
CLEANFILES+=	*~ ${KMOD}
WARNS=		2

.include <bsd.kmod.mk>
