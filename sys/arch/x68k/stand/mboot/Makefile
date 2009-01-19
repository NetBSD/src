#	$NetBSD: Makefile,v 1.14.70.1 2009/01/19 13:17:08 skrll Exp $

BOOT=		mboot
VERSION=	0.1

# text address
TEXT=		00002000
TEXTSZ=		1024

PROG=		${BOOT}
BINDIR=		/usr/mdec
BINMODE=	444
NOMAN=		# defined
STRIPFLAG=

STRIP?=		/usr/bin/strip

SRCS=		srt0.S mboot.c
KERN!=		cd ${.CURDIR}/../../../..; pwd

CPPFLAGS+=	-D_STANDALONE
CPPFLAGS+=	-nostdinc -I${KERN} -I${.CURDIR} -I.
#CPPFLAGS+=	-I${.CURDIR}/../libiocs
CPPFLAGS+=	-DTEXTADDR="0x${TEXT}" 
CPPFLAGS+=	-DBOOT=\"${BOOT}\" -DBOOT_VERS=\"${VERSION}\"
CFLAGS=		-Wno-main -Os -m68000

LINKFLAGS=	-N -static -Ttext ${TEXT}

.include "../Makefile.booters"

realall: ${PROG}
${BOOT}:	${OBJS}
	${_MKTARGET_LINK}
	${LD} ${LINKFLAGS} -o ${BOOT}.x ${OBJS} ${LDLIBS}
#	${STRIP} ${BOOT}.x
#	dd bs=32 skip=1 count=32 if=${BOOT}.x of=${BOOT}
	${OBJCOPY} -O binary ${BOOT}.x ${BOOT}
	@rm ${BOOT}.x
CLEANFILES+=	${BOOT}.x

.include <bsd.prog.mk>
