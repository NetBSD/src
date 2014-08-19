#	$NetBSD: Makefile,v 1.16.2.4 2014/08/19 23:45:38 tls Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
SSHRAMDISKDIR!=	cd ${.CURDIR}/../sshramdisk && ${PRINTOBJDIR}
SSHRAMDISK=	${SSHRAMDISKDIR}/sshramdisk.fs

.if ${MACHINE_ARCH} == "arm"
# Little endian platforms.
MDSETTARGETS=		ADI_BRH_INSTALL		${RAMDISK}	-	\
			CP3100_INSTALL		${RAMDISK}	-	\
			GEMINI_INSTALL		${RAMDISK}	-	\
			GUMSTIX_INSTALL		${RAMDISK}	-	\
			INTEGRATOR_INSTALL	${RAMDISK}	-	\
			IQ80310_INSTALL		${RAMDISK}	-	\
			IQ80321_INSTALL		${RAMDISK}	-	\
			MINI2440_INSTALL	${RAMDISK}	-	\
			OPENRD_INSTALL		${RAMDISK}	-	\
			TS7200_INSTALL		${RAMDISK}	- 	\
			TEAMASA_NPWR_INSTALL	${RAMDISK}	-	\
			KUROBOX_PRO_INSTALL	${RAMDISK}	-
.elif ${MACHINE_ARCH} == "armeb"
# Big endian platforms.
MDSETTARGETS=		ADI_BRH_INSTALL		${RAMDISK}	-
.endif

.if \
    ${MACHINE_ARCH} == "arm" || \
    ${MACHINE_ARCH} == "earm" || \
    ${MACHINE_ARCH} == "earmhf" || \
    ${MACHINE_ARCH} == "earmv6" || \
    ${MACHINE_ARCH} == "earmv6hf"
# Little endian (any ABI) ARMv6 platforms.
MDSETTARGETS+=		RPI_INSTALL		${SSHRAMDISK}	-
.endif

.if \
    ${MACHINE_ARCH} == "arm" || \
    ${MACHINE_ARCH} == "earm" || \
    ${MACHINE_ARCH} == "earmhf" || \
    ${MACHINE_ARCH} == "earmv7" || \
    ${MACHINE_ARCH} == "earmv7hf"
# Little endian (any ABI) ARMv7 platforms.
MDSETTARGETS+=		BCM5301X_INSTALL	${RAMDISK}	-	\
			BEAGLEBONE_INSTALL	${RAMDISK}	-	\
			CUBIEBOARD_INSTALL	${RAMDISK}	-
.endif

MDSET_RELEASEDIR=	installation/instkernel

MDSET_SUFFIXES.-=	srec create-srec  bin create-bin
create-srec=		${OBJCOPY} -O srec ${.TARGET:R} ${.TARGET}
create-bin=		${OBJCOPY} -O binary ${.TARGET:R} ${.TARGET}


.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
