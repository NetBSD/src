#	$NetBSD: Makefile,v 1.1.8.2 2014/08/19 23:45:40 tls Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

MKBOOT = ${TOOLDIR}/bin/${_TOOL_PREFIX}hppa-mkboot

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		RAMDISK		${RAMDISK}	-
MDSET_RELEASEDIR=	binary/kernel

LIF=	netinstall.lif

MDSET_POST = \
	${MKBOOT} ${DESTDIR}/usr/mdec/boot ${.TARGET} ${LIF}

.include "${DISTRIBDIR}/common/Makefile.mdset"

release::
	${RELEASE_INSTALL} ${LIF} ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation

.include <bsd.prog.mk>
