#	$NetBSD: Makefile,v 1.19 2020/08/10 07:00:49 rin Exp $

NOMAN=		# defined
.include <bsd.own.mk>

BOOT=		mboot
VERSION=	0.1

# text address
TEXT=		00002000
TEXTSZ=		1024

PROG=		${BOOT}
BINDIR=		/usr/mdec
BINMODE=	444
STRIPFLAG=

SRCS=		srt0.S mboot.c
KERN!=		cd ${.CURDIR}/../../../..; pwd

CPPFLAGS+=	-D_STANDALONE
CPPFLAGS+=	-nostdinc -I${KERN} -I${.CURDIR} -I.
CPPFLAGS+=	-I${.CURDIR}/../libiocs
CPPFLAGS+=	-DTEXTADDR="0x${TEXT}" 
CPPFLAGS+=	-DBOOT=\"${BOOT}\" -DBOOT_VERS=\"${VERSION}\"
CFLAGS=		-Wno-main -Os -m68000 -Wno-attributes
AFLAGS+=	-m68000

LINKFLAGS=	-N -static -Ttext ${TEXT}
LIBIOCS!=	cd ${.CURDIR}/../libiocs && ${PRINTOBJDIR}
LDLIBS+=	${LIBIOCS}/libiocs.a

.include "../Makefile.booters"

realall: ${PROG}
${BOOT}:	${OBJS} ${LDLIBS}
	${_MKTARGET_LINK}
	${LD} ${LINKFLAGS} -o ${BOOT}.x ${OBJS} ${LDLIBS}
#	${STRIP} ${BOOT}.x
#	dd bs=32 skip=1 count=32 if=${BOOT}.x of=${BOOT}
	${OBJCOPY} -O binary ${BOOT}.x ${BOOT}
	@rm ${BOOT}.x
CLEANFILES+=	${BOOT}.x

.include <bsd.prog.mk>
