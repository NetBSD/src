#	from: @(#)Makefile	5.2 (Berkeley) 3/5/91

LIB=pthread
INSTALL_PIC_ARCHIVE= no
CPPFLAGS+= -g -DPTHREAD_KERNEL -I${.CURDIR}/include -I${.CURDIR}/arch/${MACHINE}
CFLAGS+= ${CPPFLAGS}

.include "${.CURDIR}/arch/${MACHINE}/Makefile.inc"
.include "${.CURDIR}/pthreads/Makefile.inc"

all: preall

preall:
	if [ ! -e include/pthread ]; then; \
		ln -s . include/pthread; \
	fi
	if [ ! -e arch/${MACHINE}/pthread ]; then; \
		ln -s . arch/${MACHINE}/pthread; \
	fi
	
.include <bsd.lib.mk>

