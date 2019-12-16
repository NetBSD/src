#	$NetBSD: Makefile,v 1.33 2019/12/16 11:00:30 skrll Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
SSHRAMDISKDIR!=	cd ${.CURDIR}/../sshramdisk && ${PRINTOBJDIR}
SSHRAMDISK=	${SSHRAMDISKDIR}/sshramdisk.fs

.include "${NETBSDSRCDIR}/etc/etc.evbarm/Makefile.inc"

MDSETTARGETS=
.for i in ${BUILD_KERNELS:M*_INSTALL}
.if ${i} == "RPI_INSTALL"
MDSETTARGETS+=		${i}	${SSHRAMDISK}	netbsd-${i}
MDSET_SUFFIXES.netbsd-${i}=	img create-img
.else
MDSETTARGETS+=		${i}	${RAMDISK}	-
.endif
.endfor

MDSET_RELEASEDIR=	installation/instkernel

MDSET_SUFFIXES.-=	srec create-srec  img create-img
create-srec=		${OBJCOPY} -O srec ${.TARGET:R} ${.TARGET}
create-img=		${OBJCOPY} -O binary ${.TARGET:R} ${.TARGET}

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
