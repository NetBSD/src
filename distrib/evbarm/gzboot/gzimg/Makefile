#	$NetBSD: Makefile,v 1.7 2002/05/02 18:02:33 lukem Exp $

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
GZB${G}=	${GZB${K}DIR}/gzboot_${G}_${R}.sym

GZIMGS+=	gzimg_${G}_flash_${R}.gz

gzimg_${G}_flash_${R}.gz: .NOTMAIN ${GZB${G}} ${KERNOBJDIR}/${K}/${W}.bin
	@echo "Populating ${.TARGET}"
	cp ${KERNOBJDIR}/${K}/${W}.bin netbsd.tmp
	gzip -9f netbsd.tmp
	cp ${GZB${G}} gzboot.sym
	${MDSETIMAGE} -v -s gzboot.sym netbsd.tmp.gz
	${OBJCOPY} -O binary gzboot.sym gzboot.bin
	gzip -9f gzboot.bin
	mv gzboot.bin.gz ${.TARGET}
	rm gzboot.sym netbsd.tmp.gz

.endfor							# }

CLEANFILES+=	netbsd.tmp netbsd.tmp.gz gzboot.sym \
		gzboot.bin gzboot.bin.gz ${GZIMGS}


realall: ${GZIMGS}

release: check_RELEASEDIR .WAIT ${GZIMGS}
.for img in ${GZIMGS}
	${RELEASE_INSTALL} ${img} ${RELEASEDIR}/binary/gzimg
.endfor

.include <bsd.prog.mk>
