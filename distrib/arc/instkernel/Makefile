#	$NetBSD: Makefile,v 1.1 2002/04/14 14:14:16 tsutsui Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
MDSETTARGETS=	RAMDISK ${RAMDISK} -

netbsd.RAMDISK.post=	OBJCOPY=${OBJCOPY:Q} SIZE=${SIZE:Q} \
			    ${OBJCOPY} -O ecoff-littlemips \
			    -R .mdebug -R .ident \
			    netbsd.tmp netbsd.tmp.ecoff && \
			    gzip -9f netbsd.tmp.ecoff && \
			    mv netbsd.tmp.ecoff.gz netbsd.RAMDISK.ecoff.gz

CLEANFILES+=	netbsd.tmp.ecoff netbsd.RAMDISK.ecoff netbsd.RAMDISK.ecoff.gz

KERNBINDIR=	${RELEASEDIR}/binary/kernel

release: check_RELEASEDIR .WAIT netbsd.RAMDISK.gz netbsd.RAMDISK.ecoff.gz
	-mkdir -p ${KERNBINDIR}
	${RELINSTALL} netbsd.RAMDISK.gz netbsd.RAMDISK.symbols.gz \
	    netbsd.RAMDISK.ecoff.gz ${KERNBINDIR}

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
