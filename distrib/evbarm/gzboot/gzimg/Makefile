#	$NetBSD: Makefile,v 1.22.2.2 2014/08/19 23:45:38 tls Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

#
# `make' or `make all' builds all gzimg files defined in ${ARCH_TARGETS}.
# `make _KERNELNAME_' builds all gzimg files for specified kernels.
#



#	ARCH_TARGETS is a list of:
#		KERNEL_name	GZBOOT_name	GZIMG_name
#		RELOC_ADDR	WHICH_kernel
#	
.if ${MACHINE_ARCH} == "arm"
# Little endian platforms.
ARCH_TARGETS=	ADI_BRH		ADI_BRH		ADI_BRH_sd0	\
		0x00140000	netbsd-sd0			\
								\
		ADI_BRH		ADI_BRH		ADI_BRH_wd0	\
		0x00140000	netbsd-wd0			\
								\
		CP3100		IQ80321		CP3100_sd0	\
		0xf0080000	netbsd-sd0			\
								\
		CP3100		IQ80321		CP3100_wd0	\
		0xf0080000	netbsd-wd0			\
								\
		GEMINI		GEMINI		GEMINI		\
		0x01600000	netbsd				\
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
		0x00080000	netbsd-sd0			\
								\
		TS7200		TS7200		TS7200_wd0	\
		0x60660000	netbsd-wd0			\
								\
		SMDK2800	SMDK2800	SMDK2800	\
		0x00000000	netbsd
.elif ${MACHINE_ARCH} == "armeb"
# Big endian platforms.
ARCH_TARGETS=	ADI_BRH		ADI_BRH		ADI_BRH_sd0	\
		0x00140000	netbsd-sd0			\
								\
		ADI_BRH		ADI_BRH		ADI_BRH_wd0	\
		0x00140000	netbsd-wd0
.endif

# Default is to build for all ARCH_TARGETS
ALL_KERNELS?= +

.for K G I R W in ${ARCH_TARGETS}			# {
.for configsel in ${ALL_KERNELS}			# {
.if ${configsel} == "+" || ${configsel} == ${K}

GZB${G}=	${DESTDIR}/usr/mdec/gzboot_${G}_${R}.bin

GZIMGS+=	gzimg_${I}_flash_${R}.gz

gzimg_${I}_flash_${R}: .NOTMAIN ${GZB${G}} ${KERNOBJDIR}/${K}/${W}.bin
	@echo "Populating ${.TARGET}"
	-rm -f ${.TARGET}.kern.gz
	${TOOL_GZIP_N} -9c ${KERNOBJDIR}/${K}/${W}.bin > ${.TARGET}.kern.gz
	${TOOL_CAT} ${GZB${G}} ${.TARGET}.kern.gz > ${.TARGET}

gzimg_${I}_flash_${R}.gz: .NOTMAIN gzimg_${I}_flash_${R}
	-rm -f ${.TARGET}
	${TOOL_GZIP_N} -9c ${.ALLSRC} > ${.TARGET}

CLEANFILES+=	gzimg_${I}_flash_${R}.kern gzimg_${I}_flash_${R}.kern.gz \
		gzimg_${I}_flash_${R}

.if empty(GZKERNELS:M${K})
GZKERNELS+=	$K
.endif
KZ${K}+=	gzimg_${I}_flash_${R}.gz

.endif
.endfor							# }
.endfor							# }

.for K in ${GZKERNELS}
${K}: .PHONY ${KZ${K}}
.endfor

CLEANFILES+=	${GZIMGS}


realall: ${GZIMGS}

release: check_RELEASEDIR .WAIT ${GZIMGS}
.for img in ${GZIMGS}
	${RELEASE_INSTALL} ${img} ${RELEASEDIR}/${RELEASEMACHINEDIR}/binary/gzimg
.endfor

.include <bsd.prog.mk>
