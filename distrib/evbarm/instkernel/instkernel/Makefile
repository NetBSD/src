#	$NetBSD: Makefile,v 1.29 2017/12/13 11:37:37 skrll Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
SSHRAMDISKDIR!=	cd ${.CURDIR}/../sshramdisk && ${PRINTOBJDIR}
SSHRAMDISK=	${SSHRAMDISKDIR}/sshramdisk.fs

.include "${NETBSDSRCDIR}/etc/etc.evbarm/Makefile.inc"

MDSETTARGETS=
MDSET_SUFFIXES.-=	srec create-srec  bin create-bin
.for i in ${BUILD_KERNELS:M*_INSTALL}
.if ${i} == "RPI_INSTALL" || ${i} == "RPI2_INSTALL"
MDSETTARGETS+=		${i}	${SSHRAMDISK}	-
MDSET_SUFFIXES.${i}=	img create-img
.else
MDSETTARGETS+=		${i}	${RAMDISK}	-
.endif
.endfor

MDSET_RELEASEDIR=	installation/instkernel

create-srec=		${OBJCOPY} -O srec ${.TARGET:R} ${.TARGET}
create-bin=		${OBJCOPY} -O binary ${.TARGET:R} ${.TARGET}
create-img=	\
	${HOST_SH} ${NETBSDSRCDIR}/sys/arch/evbarm/compile/rpi-mkknlimg.sh \
	${.TARGET:R}.bin ${.TARGET}

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
