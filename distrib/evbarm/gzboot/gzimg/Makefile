#	$NetBSD: Makefile,v 1.4 2002/04/24 21:09:57 thorpej Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

.include <bsd.kernobj.mk>


#	TARGETS is a list of:
#		KERNEL_name	RELOC_ADDR	WHICH_kernel
#	
TARGETS=	IQ80310		0x00080000	netbsd		\
		IQ80321		0xf0080000	netbsd

.for K R W in ${TARGETS}				# {

GZB${K}DIR!=	cd ${.CURDIR}/../gzboot_${K}_flash_${R} && ${PRINTOBJDIR}
GZB${K}=	${GZB${K}DIR}/gzboot_${K}_${R}.sym

GZIMGS+=	gzimg_${K}_flash_${R}.gz

gzimg_${K}_flash_${R}.gz: .NOTMAIN ${GZB${K}} ${KERNOBJDIR}/${K}/${W}.bin
	@echo "Populating ${.TARGET}"
	cp ${KERNOBJDIR}/${K}/${W}.bin netbsd.tmp
	gzip -9f netbsd.tmp
	cp ${GZB${K}} gzboot.sym
	${MDSETIMAGE} -v -s gzboot.sym netbsd.tmp.gz
	${OBJCOPY} -O binary gzboot.sym gzboot.bin
	gzip -9f gzboot.bin
	mv gzboot.bin.gz ${.TARGET}

.endfor							# }

CLEANFILES+=	netbsd.tmp netbsd.tmp.gz gzboot.sym \
		gzboot.bin gzboot.bin.gz ${GZIMGS}


#	do the work
#
realall: ${GZIMGS}

ITARGET=	${RELEASEDIR}/binary/gzimg

release: check_RELEASEDIR
.for img in ${GZIMGS}
	${RELINSTALL} ${img} ${ITARGET}/.
.endfor

.include <bsd.prog.mk>
