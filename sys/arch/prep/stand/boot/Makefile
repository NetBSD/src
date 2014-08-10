#	$NetBSD: Makefile,v 1.28.2.1 2014/08/10 06:54:06 tls Exp $

NOMAN= # defined

.include <bsd.own.mk>

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
SRCS+= kbd.c ns16550.c vers.c vreset.c vga.c 
SRCS+= pci.c sd.c siop.c

CFLAGS= -Os -ffreestanding
CFLAGS+= -Werror -Wall -Wstrict-prototypes -Wmissing-prototypes
AFLAGS= -x assembler-with-cpp

CPPFLAGS= -nostdinc -I${.OBJDIR} -I${MKBOOTIMAGE} -I${S}
CPPFLAGS+= -D_STANDALONE -DRELOC=${RELOC} -DUSE_SCAN
CPPFLAGS+= -D__daddr_t=int32_t
#CPPFLAGS+= -DDBMONITOR

.if (${BASE} == "boot")
CPPFLAGS+= -DCONS_VGA -DVGA_RESET
.elif (${BASE} == "boot_com0")
CPPFLAGS+= -DCONS_SERIAL -DCOMSPEED=9600 -DCOMPORT=0x3f8
.elif (${BASE} == "boot_com0_vreset")
CPPFLAGS+= -DCONS_SERIAL -DCOMSPEED=9600 -DCOMPORT=0x3f8 -DVGA_RESET
.endif

STRIPFLAG=
BINMODE= 444

# XXX SHOULD NOT NEED TO DEFINE THESE!
LIBCRT0=
LIBCRTI=
LIBC=
LIBCRTBEGIN=
LIBCRTEND=

RELOC= 0x800000

CLEANFILES+= vers.c machine powerpc ${PROG}.elf

LIBS= ${L}/sa/libsa.a ${L}/kern/libkern.a ${L}/z/libz.a

vers.c: ${.CURDIR}/../boot/version
	${HOST_SH} ${S}/conf/newvers_stand.sh ${${MKREPRO} == "yes" :?:-D} \
	    ${.CURDIR}/../boot/version "prep" ${NEWVERSWHAT}

${PROG}: ${OBJS} ${LIBS}
	${_MKTARGET_LINK}
	${LD} -o ${PROG}.elf -N -T ${.CURDIR}/../boot/ld.script \
		-Ttext ${RELOC} ${OBJS} ${LIBS}
	${STRIP} -o ${PROG} ${PROG}.elf

.include <bsd.prog.mk>
.include <bsd.klinks.mk>
