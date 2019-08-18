#	$NetBSD: Makefile,v 1.31.2.1 2019/08/18 13:14:26 msaitoh Exp $

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
MDSET_SUFFIXES.netbsd-${i}=	bin create-bin  img create-img
MDSET_.RPI_INSTALL.netbsd-RPI_INSTALL.img.deps=	netbsd-RPI_INSTALL.bin
.else
MDSETTARGETS+=		${i}	${RAMDISK}	-
.endif
.endfor

MDSET_RELEASEDIR=	installation/instkernel

MDSET_SUFFIXES.-=	srec create-srec  bin create-bin
create-srec=		${OBJCOPY} -O srec ${.TARGET:R} ${.TARGET}
create-bin=		${OBJCOPY} -O binary ${.TARGET:R} ${.TARGET}
create-img=	\
	${HOST_SH} ${NETBSDSRCDIR}/sys/arch/evbarm/compile/rpi-mkknlimg.sh \
	${.TARGET:R}.bin ${.TARGET}

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
