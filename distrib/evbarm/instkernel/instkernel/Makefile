#	$NetBSD: Makefile,v 1.5 2002/05/02 18:02:33 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
MDSETTARGETS=	TEAMASA_NPWR_INSTALL ${RAMDISK} -

MDSET.-.suffixes= srec bin
MDSET.-.post=	${OBJCOPY} -O srec netbsd.tmp netbsd.srec && \
		    gzip -9f netbsd.srec ; \
		${OBJCOPY} -O binary netbsd.tmp netbsd.bin && \
		    gzip -9f netbsd.bin

MDSET_RELEASEDIR=	installation/instkernel

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
