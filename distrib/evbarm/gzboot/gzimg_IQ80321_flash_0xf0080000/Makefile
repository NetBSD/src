#	$NetBSD: Makefile,v 1.2 2002/03/29 01:52:46 thorpej Exp $

.include "../../../../Makefile.inc"
.include <bsd.own.mk>

KIMGDIR!=	cd ${.CURDIR}/../kern_IQ80321 && ${PRINTOBJDIR}
KIMG=		${KIMGDIR}/netbsd.bin.gz

GZBDIR!=	cd ${.CURDIR}/../gzboot_IQ80321_flash_0xf0080000 && \
		    ${PRINTOBJDIR}
GZB=		${GZBDIR}/gzboot_IQ80321_0xf0080000.sym

GZIMG=		gzimg_IQ80321_flash_0xf0080000

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
