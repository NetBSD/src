#	$NetBSD: Makefile,v 1.8 2002/05/05 14:37:38 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>


#	TARGETS is a list of:
#		KERNEL_name	GZBOOT_name	RELOC_ADDR	WHICH_kernel
#	
TARGETS=	IQ80310		IQ80310		0x00080000	netbsd     \
		IQ80321		IQ80321		0xf0080000	netbsd

.for K G R W in ${TARGETS}				# {

GZB${G}DIR!=	cd ${.CURDIR}/../gzboot_${G}_flash_${R} && ${PRINTOBJDIR}
GZB${G}=	${GZB${G}DIR}/gzboot_${G}_${R}.sym

GZIMGS+=	gzimg_${G}_flash_${R}.gz

gzimg_${G}_flash_${R}.gz: .NOTMAIN ${GZB${G}} ${KERNOBJDIR}/${K}/${W}.bin
	@echo "Populating ${.TARGET}"
	cp ${KERNOBJDIR}/${K}/${W}.bin ${.TARGET:R}.kern
	gzip -9f ${.TARGET:R}.kern
	cp ${GZB${G}} ${.TARGET:R}
	${MDSETIMAGE} -v -s ${.TARGET:R} ${.TARGET:R}.kern.gz
	${OBJCOPY} -O binary ${.TARGET:R} ${.TARGET:R}
	gzip -9f ${.TARGET:R}

CLEANFILES+=	gzimg_${G}_flash_${R}.kern gzimg_${G}_flash_${R}.kern.gz \
		gzimg_${G}_flash_${R}

.endfor							# }

CLEANFILES+=	${GZIMGS}


realall: ${GZIMGS}

release: check_RELEASEDIR .WAIT ${GZIMGS}
.for img in ${GZIMGS}
	${RELEASE_INSTALL} ${img} ${RELEASEDIR}/binary/gzimg
.endfor

.include <bsd.prog.mk>
