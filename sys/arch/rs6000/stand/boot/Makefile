#	$NetBSD: Makefile,v 1.6.2.1 2014/08/10 06:54:06 tls Exp $

NOMAN= # defined

.include <bsd.own.mk>
.include <bsd.klinks.mk>

COMMON= ${.CURDIR}/../common
COMMONOBJ!=	cd ${COMMON} && ${PRINTOBJDIR}

.PATH: ${.CURDIR}/../boot ${COMMONOBJ}

S= ${.CURDIR}/../../../..
L= ${COMMONOBJ}/lib

MKBOOTIMAGE= ${.CURDIR}/../../../powerpc/stand/mkbootimage

BASE?=		boot
PROG=		${BASE}
NEWVERSWHAT=	"BOOT"

SRCS= srt0.s
SRCS+= boot.c clock.c com.c conf.c cons.c devopen.c
SRCS+= filesystem.c inkernel.c io.c tgets.c prf.c monitor.c
SRCS+= ns16550.c vers.c
SRCS+= iplcb.c

CFLAGS= -Os -ffreestanding
CFLAGS+= -Werror -Wall -Wstrict-prototypes -Wmissing-prototypes
AFLAGS= -x assembler-with-cpp

CPPFLAGS= -nostdinc -I${.OBJDIR} -I${MKBOOTIMAGE} -I${S}
CPPFLAGS+= -D_STANDALONE -DRELOC=${RELOC} -DUSE_SCAN
CPPFLAGS+= -D__daddr_t=int32_t
#CPPFLAGS+= -DDBMONITOR
CPPFLAGS+= -DCONS_SERIAL -DCOMSPEED=9600 -DCOMPORT=0x30

STRIPFLAG=
BINMODE= 444

# XXX SHOULD NOT NEED TO DEFINE THESE!
LIBCRT0=
LIBCRTI=
LIBC=
LIBCRTBEGIN=
LIBCRTEND=

RELOC= 0x800000
#RELOC= 0x0

CLEANFILES+= vers.c

LIBS= ${L}/sa/libsa.a ${L}/kern/libkern.a ${L}/z/libz.a

vers.c: ${.CURDIR}/../boot/version
	${HOST_SH} ${S}/conf/newvers_stand.sh ${${MKREPRO} == "yes" :?:-D} \
	    ${.CURDIR}/../boot/version "rs6000" ${NEWVERSWHAT}

${PROG}: ${OBJS} ${LIBS}
	${_MKTARGET_LINK}
	${LD} -o ${PROG} -s -N -T ${.CURDIR}/../boot/ld.script \
		-Ttext ${RELOC} ${OBJS} ${LIBS}

.include <bsd.prog.mk>
