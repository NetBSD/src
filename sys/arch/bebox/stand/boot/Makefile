#	$NetBSD: Makefile,v 1.29 2010/10/14 06:58:22 kiyohara Exp $

NOMAN= # defined

.include <bsd.own.mk>

COMMON= ${.CURDIR}/../common
COMMONOBJ!=	cd ${COMMON} && ${PRINTOBJDIR}

.PATH: ${.CURDIR}/../boot ${COMMONOBJ} ${.CURDIR}/../../../powerpc/powerpc

S= ${.CURDIR}/../../../..
L= ${COMMONOBJ}/lib

ELF2PEF= ${.CURDIR}/../../../powerpc/stand/mkbootimage

COMMONLIBDIR= ${.CURDIR}/../../../../../common

BASE?=		boot
PROG=		${BASE}
NEWVERSWHAT=	"BOOT"

SRCS= srt0.s
SRCS+= boot.c clock.c com.c conf.c cons.c cpu.c devopen.c
SRCS+= fd.c filesystem.c inkernel.c io.c kbd.c monitor.c ns16550.c
SRCS+= pci.c prf.c sd.c siop.c tgets.c vers.c vga.c video.c vreset.c wdc.c wd.c
SRCS+= setjmp.S

CFLAGS= -Wno-main -ffreestanding
AFLAGS= -x assembler-with-cpp -traditional-cpp

CPPFLAGS= -nostdinc
CPPFLAGS+= -I${.OBJDIR} -I${S} -I${COMMONLIBDIR}/include -I${S}/lib/libsa
CPPFLAGS+= -I${ELF2PEF}
CPPFLAGS+= -Werror -Wall -Wstrict-prototypes -Wmissing-prototypes
CPPFLAGS+= -D_STANDALONE -DRELOC=${RELOC} -DUSE_SCAN
CPPFLAGS+= -D__daddr_t=int32_t
#CPPFLAGS+= -DDBMONITOR

.if (${BASE} == "boot")
CPPFLAGS+= -DCONS_BE
.elif (${BASE} == "boot_com0")
CPPFLAGS+= -DCONS_SERIAL -DCOMSPEED=9600 -DCOMPORT=0x3f8
.elif (${BASE} == "boot_vga")
CPPFLAGS+= -DCONS_VGA
.endif

STRIPFLAG=
BINMODE= 444

# XXX SHOULD NOT NEED TO DEFINE THESE!
LIBCRT0=
LIBC=
LIBCRTBEGIN=
LIBCRTEND=

RELOC= 0x700000

CLEANFILES+= vers.c machine powerpc ${PROG}.elf

LIBS= ${L}/sa/libsa.a ${L}/kern/libkern.a ${L}/z/libz.a

.if !make(obj) && !make(clean) && !make(cleandir)
.BEGIN: machine powerpc
.NOPATH: machine powerpc

machine::
	-rm -f $@
	ln -s ${S}/arch/${MACHINE}/include $@

powerpc::
	-rm -f $@
	ln -s ${S}/arch/powerpc/include $@
.endif

vers.c: ${.CURDIR}/../boot/version
	${HOST_SH} ${S}/conf/newvers_stand.sh ${.CURDIR}/../boot/version \
		"bebox" ${NEWVERSWHAT}

${PROG}: ${OBJS} ${LIBS}
	${LD} -o ${PROG}.elf -N -T ${.CURDIR}/../boot/ld.script \
		-Ttext ${RELOC} ${OBJS} ${LIBS}
	${STRIP} -o ${PROG} ${PROG}.elf

.include <bsd.prog.mk>
