#	$NetBSD: Makefile,v 1.1 2002/04/11 12:19:43 lukem Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

.include <bsd.kernobj.mk>


#	TARGETS is a list of:
#		KERNEL_name	RELOC_ADDR
#	
TARGETS=	IQ80310		0x00080000	\
		IQ80321		0xf0080000

.for K R in ${TARGETS}					# {

GZB${K}DIR!=	cd ${.CURDIR}/../gzboot_${K}_flash_${R} && ${PRINTOBJDIR}
GZB${K}=	${GZB${K}DIR}/gzboot_${K}_${R}.sym

GZIMGS+=	gzimg_${K}_flash_${R}.gz

gzimg_${K}_flash_${R}.gz: .NOTMAIN ${GZB${K}} ${KERNOBJDIR}/${K}/netbsd.bin
	@echo "Populating ${.TARGET}"
	cp ${KERNOBJDIR}/${K}/netbsd.bin netbsd.tmp
	gzip -9 netbsd.tmp
	cp ${GZB${K}} gzboot.sym
	${MDSETIMAGE} -v -s gzboot.sym netbsd.tmp.gz
	${OBJCOPY} -O binary gzboot.sym gzboot.bin
	gzip -9 gzboot.bin
	mv gzboot.bin.gz ${.TARGET}

.endfor							# }

CLEANFILES+=	netbsd.tmp netbsd.tmp.gz gzboot.sym \
		gzboot.bin gzboot.bin.gz ${GZIMGS}


#	do the work
#
realall: ${GZIMGS}

.include <bsd.prog.mk>
