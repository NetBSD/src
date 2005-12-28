#	$NetBSD: Makefile,v 1.17 2005/12/28 09:10:26 skrll Exp $

NOMAN= # defined

.include <bsd.own.mk>

COMMON= ${.CURDIR}/../common
COMMONOBJ!=	cd ${COMMON} && ${PRINTOBJDIR}

.PATH: ${.CURDIR}/../boot ${COMMONOBJ}

S= ${.CURDIR}/../../../..
L= ${COMMONOBJ}/lib

BASE?=		boot
PROG=		${BASE}
NEWVERSWHAT=	"BOOT"

SRCS= srt0.s
SRCS+= boot.c clock.c com.c conf.c cons.c devopen.c fd.c
SRCS+= filesystem.c inkernel.c io.c tgets.c prf.c monitor.c
SRCS+= kbd.c ns16550.c vers.c vreset.c vga.c video.c

CFLAGS= -Os -mmultiple -ffreestanding
AFLAGS= -x assembler-with-cpp -traditional-cpp

CPPFLAGS= -nostdinc -I${.OBJDIR} -I${.CURDIR}/../mkbootimage -I${S}
CPPFLAGS+= -Werror -Wall -Wstrict-prototypes -Wmissing-prototypes
CPPFLAGS+= -D_STANDALONE -DDBMONITOR -DRELOC=${RELOC} -DUSE_SCAN
CPPFLAGS+= -D__daddr_t=int32_t

.if (${BASE} == "boot")
CPPFLAGS+= -DCONS_VGA
#CPPFLAGS+= -DCONS_FB
.elif (${BASE} == "boot_com0")
CPPFLAGS+= -DCONS_SERIAL -DCOMSPEED=9600 -DCOMPORT=0x3f8
.endif

STRIPFLAG=
BINMODE= 444

# XXX SHOULD NOT NEED TO DEFINE THESE!
LIBCRT0=
LIBC=
LIBCRTBEGIN=
LIBCRTEND=

RELOC= 0x800000

CLEANFILES+= vers.c machine powerpc

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
	${HOST_SH} ${S}/conf/newvers_stand.sh ${.CURDIR}/../boot/version "prep" ${NEWVERSWHAT}

${PROG}: ${OBJS} ${LIBS}
	${LD} -o ${PROG} -s -N -T ${.CURDIR}/../boot/ld.script \
		-Ttext ${RELOC} ${OBJS} ${LIBS}

.include <bsd.prog.mk>
