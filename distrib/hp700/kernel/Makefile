#	$NetBSD: Makefile,v 1.3 2008/06/23 02:16:53 matt Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

MKBOOT = ${TOOLDIR}/bin/${_TOOL_PREFIX}hp700-mkboot

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
