#	$NetBSD: Makefile,v 1.2 2002/03/29 01:52:46 thorpej Exp $

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

clean cleandir distclean:
	rm -f gzboot.sym ${GZIMG} ${GZIMG}.gz

# nothing to do here
depend includes lint regress tags:

.include <bsd.obj.mk>
