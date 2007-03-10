#	$NetBSD: Makefile,v 1.15 2007/03/10 17:57:41 jmmv Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		INSTALL ${RAMDISK} -
MDSET_RELEASEDIR=	binary/kernel

MDSET_POST= \
	${OBJCOPY} -O a.out-arm-netbsd -R .ident -R .arm.atpcs -R .comment \
	    ${.TARGET} ${.TARGET}.aout; \
	gzip -9c ${.TARGET}.aout >${.TARGET}.aout.gz

.include "${DISTRIBDIR}/common/Makefile.mdset"

.PHONY: release-aout-instkernel
release-aout-instkernel:
	${RELEASE_INSTALL} netbsd-INSTALL.aout.gz \
	    ${RELEASEDIR}/${MACHINE}/installation/instkernel/netbsd.gz

.include <bsd.prog.mk>
