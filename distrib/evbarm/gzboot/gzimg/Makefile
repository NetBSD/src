#	$NetBSD: Makefile,v 1.11 2003/04/08 21:26:59 thorpej Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>


#	TARGETS is a list of:
#		KERNEL_name	GZBOOT_name	RELOC_ADDR	WHICH_kernel
#	
TARGETS=	ADI_BRH		ADI_BRH_sd0	0x00140000	netbsd-sd0   \
		ADI_BRH		ADI_BRH_wd0	0x00140000	netbsd-wd0   \
		IQ80310		IQ80310_sd0	0x00080000	netbsd-sd0   \
		IQ80310		IQ80310_wd0	0x00080000	netbsd-wd0   \
		IQ80321		IQ80321_sd0	0xf0080000	netbsd-sd0   \
		IQ80321		IQ80321_wd0	0xf0080000	netbsd-wd0   \
		TEAMASA_NPWR	TEAMASA_NPWR_sd0 0x00080000	netbsd-sd0

.for K G R W in ${TARGETS}				# {

GZB${G}DIR!=	cd ${.CURDIR}/../gzboot_${G}_flash_${R} && ${PRINTOBJDIR}
GZB${G}=	${GZB${G}DIR}/gzboot_${G}_${R}.sym

GZIMGS+=	gzimg_${G}_flash_${R}.gz

gzimg_${G}_flash_${R}: .NOTMAIN ${GZB${G}} ${KERNOBJDIR}/${K}/${W}.bin
	@echo "Populating ${.TARGET}"
	-rm -f ${.TARGET}.kern.gz
	gzip -9c ${KERNOBJDIR}/${K}/${W}.bin > ${.TARGET}.kern.gz
	cp ${GZB${G}} ${.TARGET}
	${MDSETIMAGE} -v -s ${.TARGET} ${.TARGET}.kern.gz
	${OBJCOPY} -O binary ${.TARGET} ${.TARGET}

gzimg_${G}_flash_${R}.gz: .NOTMAIN gzimg_${G}_flash_${R}
	-rm -f ${.TARGET}
	gzip -9c ${.ALLSRC} > ${.TARGET}

CLEANFILES+=	gzimg_${G}_flash_${R}.kern gzimg_${G}_flash_${R}.kern.gz \
		gzimg_${G}_flash_${R}

.endfor							# }

CLEANFILES+=	${GZIMGS}


realall: ${GZIMGS}

release: check_RELEASEDIR .WAIT ${GZIMGS}
.for img in ${GZIMGS}
	${RELEASE_INSTALL} ${img} ${RELEASEDIR}/${MACHINE}/binary/gzimg
.endfor

.include <bsd.prog.mk>
