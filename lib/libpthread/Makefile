#	from: @(#)Makefile	5.2 (Berkeley) 3/5/91

LIB=pthread
INSTALL_PIC_ARCHIVE= no
CPPFLAGS+= -g -DPTHREAD_KERNEL -I. -I${.CURDIR}/include -I${.CURDIR}/arch/${MACHINE}
CFLAGS+= ${CPPFLAGS}

.include "${.CURDIR}/arch/${MACHINE}/Makefile.inc"
.include "${.CURDIR}/pthreads/Makefile.inc"

all: includelinks

beforedepend: includelinks

CLEANFILES+=pthread 

includelinks: 
	if [ ! -e pthread ]; then; \
		ln -s ${.CURDIR}/include pthread; \
	fi
	
.include <bsd.lib.mk>

