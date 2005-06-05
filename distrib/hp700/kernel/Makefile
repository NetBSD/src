#	$NetBSD: Makefile,v 1.2 2005/06/05 18:56:51 chs Exp $

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
	${RELEASE_INSTALL} ${LIF} ${RELEASEDIR}/${MACHINE}/installation

.include <bsd.prog.mk>
