#	$NetBSD: Makefile,v 1.15 2003/07/10 10:34:05 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>


#	TARGETS is a list of:
#		KERNEL_name	GZBOOT_name	GZIMG_name
#		RELOC_ADDR	WHICH_kernel
#	
.if ${MACHINE_ARCH} == "arm"
# Little endian platforms.
TARGETS=	ADI_BRH		ADI_BRH		ADI_BRH_sd0	\
		0x00140000	netbsd-sd0			\
								\
		ADI_BRH		ADI_BRH		ADI_BRH_wd0	\
		0x00140000	netbsd-wd0			\
								\
		IQ80310		IQ80310		IQ80310_sd0	\
		0x00080000	netbsd-sd0			\
								\
		IQ80310		IQ80310		IQ80310_wd0	\
		0x00080000	netbsd-wd0			\
								\
		IQ80321		IQ80321		IQ80321_sd0	\
		0xf0080000	netbsd-sd0			\
								\
		IQ80321		IQ80321		IQ80321_wd0	\
		0xf0080000	netbsd-wd0			\
								\
		TEAMASA_NPWR	IQ80310		TEAMASA_NPWR_sd0 \
		0x00080000	netbsd-sd0
.else
# Big endian platforms.
TARGETS=	ADI_BRH		ADI_BRH		ADI_BRH_sd0	\
		0x00140000	netbsd-sd0			\
								\
		ADI_BRH		ADI_BRH		ADI_BRH_wd0	\
		0x00140000	netbsd-wd0
.endif

.for K G I R W in ${TARGETS}				# {

GZB${G}=	${DESTDIR}/usr/mdec/gzboot_${G}_${R}.bin

GZIMGS+=	gzimg_${I}_flash_${R}.gz

gzimg_${I}_flash_${R}: .NOTMAIN ${GZB${G}} ${KERNOBJDIR}/${K}/${W}.bin
	@echo "Populating ${.TARGET}"
	-rm -f ${.TARGET}.kern.gz
	gzip -9c ${KERNOBJDIR}/${K}/${W}.bin > ${.TARGET}.kern.gz
	${TOOL_CAT} ${GZB${G}} ${.TARGET}.kern.gz > ${.TARGET}

gzimg_${I}_flash_${R}.gz: .NOTMAIN gzimg_${I}_flash_${R}
	-rm -f ${.TARGET}
	gzip -9c ${.ALLSRC} > ${.TARGET}

CLEANFILES+=	gzimg_${I}_flash_${R}.kern gzimg_${I}_flash_${R}.kern.gz \
		gzimg_${I}_flash_${R}

.endfor							# }

CLEANFILES+=	${GZIMGS}


realall: ${GZIMGS}

release: check_RELEASEDIR .WAIT ${GZIMGS}
.for img in ${GZIMGS}
	${RELEASE_INSTALL} ${img} ${RELEASEDIR}/${MACHINE}/binary/gzimg
.endfor

.include <bsd.prog.mk>
