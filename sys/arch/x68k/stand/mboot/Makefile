#	$NetBSD: Makefile,v 1.7.4.1 2001/10/01 12:43:10 fvdl Exp $

BOOT=		mboot
VERSION=	0.1

# text address
TEXT=		00002000
TEXTSZ=		1024

PROG=		${BOOT}
BINDIR=		/usr/mdec
BINMODE=	444
MKMAN=		no
STRIPFLAG=

STRIP?=		/usr/bin/strip

SRCS=		srt0.S mboot.c
KERN!=		cd ${.CURDIR}/../../../..; pwd

CPPFLAGS+=	-nostdinc -I${KERN} -I${.CURDIR} -I.
#CPPFLAGS+=	-I${.CURDIR}/../libiocs
CPPFLAGS+=	-DTEXTADDR="0x${TEXT}" 
CPPFLAGS+=	-DBOOT=\"${BOOT}\" -DBOOT_VERS=\"${VERSION}\"
CFLAGS=		-Wno-main -Os -m68000

LDFLAGS=	-N -static -Ttext ${TEXT}

.PHONY:	machine-links
beforedepend: machine-links
machine-links:
	echo ${.CURDIR}
	-rm -f machine && \
	    ln -s ${KERN}/arch/${MACHINE}/include machine
	-rm -f ${MACHINE_ARCH} && \
	    ln -s ${KERN}/arch/${MACHINE_ARCH}/include ${MACHINE_ARCH}
CLEANFILES+=	machine ${MACHINE_ARCH}

realall: machine-links ${PROG}
${BOOT}:	${OBJS}
	${LD} ${LDFLAGS} -o ${BOOT}.x ${OBJS} ${LDLIBS}
#	${STRIP} ${BOOT}.x
#	dd bs=32 skip=1 count=32 if=${BOOT}.x of=${BOOT}
	${OBJCOPY} -O binary ${BOOT}.x ${BOOT}
	@rm ${BOOT}.x
CLEANFILES+=	${BOOT}.x

.include <bsd.prog.mk>
