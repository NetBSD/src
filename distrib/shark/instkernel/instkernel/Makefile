#	$NetBSD: Makefile,v 1.24 2019/01/02 17:34:03 christos Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		INSTALL ${RAMDISK} -
MDSET_RELEASEDIR=	binary/kernel

MDSET_SUFFIXES.-=	aout create-aout

create-aout= \
	${ARM_ELF2AOUT} ${.TARGET:R} ${.TARGET}

release::	check_RELEASEDIR .WAIT netbsd-INSTALL.aout.gz
	${RELEASE_INSTALL} netbsd-INSTALL.aout.gz \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/instkernel/netbsd.gz

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
