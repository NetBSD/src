#	$NetBSD: Makefile,v 1.6 2002/05/03 15:22:58 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
MDSETTARGETS=	TEAMASA_NPWR_INSTALL ${RAMDISK} -

MDSET.-.suffixes= srec bin
MDSET.-.post=	${OBJCOPY} -O srec ${.TARGET:R} ${.TARGET:R}.srec && \
		gzip -9f ${.TARGET:R}.srec ; \
		${OBJCOPY} -O binary ${.TARGET:R} ${.TARGET:R}.bin && \
		gzip -9f ${.TARGET:R}.bin

MDSET_RELEASEDIR=	installation/instkernel

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
