#	$NetBSD: Makefile,v 1.1 2002/03/28 22:03:02 thorpej Exp $

.include "../../../../Makefile.inc"
.include <bsd.own.mk>

KIMGDIR!=	cd ${.CURDIR}/../kern_IQ80310 && ${PRINTOBJDIR}
KIMG=		${KIMGDIR}/netbsd.bin.gz

GZBDIR!=	cd ${.CURDIR}/../gzboot_IQ80310_flash_0x00080000 && \
		    ${PRINTOBJDIR}
GZB=		${GZBDIR}/gzboot_IQ80310_0x00080000.sym

GZIMG=		gzimg_IQ80310_flash_0x00080000

all:
	cp ${GZB} gzboot.sym
	${MDSETIMAGE} -v -s gzboot.sym ${KIMG}
	${OBJCOPY} -O binary gzboot.sym ${GZIMG}
	gzip -9f ${GZIMG}

release: check_RELEASEDIR
	-mkdir -p ${RELEASEDIR}/binary/gzimg
	cp -p ${GZIMG}.gz ${RELEASEDIR}/binary/gzimg/${GZIMG}.gz

clean cleandir distclean:
	rm -f gzboot.sym ${GZIMG} ${GZIMG}.gz

# nothing to do here
depend includes lint regress tags:

.include <bsd.obj.mk>
