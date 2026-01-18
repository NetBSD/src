#	$NetBSD: Makefile,v 1.1 2026/01/18 11:34:12 skrll Exp $

DTSSUBDIR=	xilinx
.if !empty(MACHINE_ARCH:Mearmv7*)
DTSMAKEVARS=	CONFIG_ARCH_ZYNQ=y
.endif
DTSFILESCMD=	${MAKE} -C ${ARCHDTSDIR}/${DTSSUBDIR} ${DTSMAKEVARS} -v dtb-y
DTS=		${DTSFILESCMD:sh}

.include <bsd.dtb.mk>
#	$NetBSD: Makefile,v 1.1 2026/01/18 11:34:12 skrll Exp $

DTSSUBDIR=	xilinx
.if !empty(MACHINE_ARCH:Mearmv7*)
DTSMAKEVARS=	CONFIG_ARCH_ZYNQ=y
.endif
DTSFILESCMD=	${MAKE} -C ${ARCHDTSDIR}/${DTSSUBDIR} ${DTSMAKEVARS} -v dtb-y
DTS=		${DTSFILESCMD:sh}

.include <bsd.dtb.mk>
