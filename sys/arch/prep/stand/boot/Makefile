#	$NetBSD: Makefile,v 1.5 2001/06/19 11:56:28 nonaka Exp $

COMMON= ${.CURDIR}/../common
.ifnmake(print-objdir)
COMMONOBJ!=	cd ${COMMON}; ${MAKE} print-objdir
.endif

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

AFLAGS= -x assembler-with-cpp -traditional-cpp

CPPFLAGS= -nostdinc -I${.OBJDIR} -I${.CURDIR}/../mkbootimage -I${S}
CPPFLAGS+= -Werror -Wall -Wstrict-prototypes -Wmissing-prototypes
CPPFLAGS+= -D_STANDALONE -DDBMONITOR -DRELOC=${RELOC} -DUSE_SCAN

.if (${BASE} == "boot")
CPPFLAGS+= -DCONS_VGA
#CPPFLAGS+= -DCONS_FB
.elif (${BASE} == "boot_com0")
CPPFLAGS+= -DCONS_SERIAL -DCOMSPEED=9600 -DCOMPORT=0x3f8
.endif

OBJDUMP?= objdump
MKMAN= no
STRIPFLAG=
BINMODE= 444

RELOC= 0x800000
HEAD_SIZE= `${OBJDUMP} -h ${.OBJDIR}/${PROG} | grep \.text | awk '{print $$6}'`
KERN_OFFSET= `ls -l ${.OBJDIR}/${PROG} | awk '{print $$5}'`

CLEANFILES+= vers.c machine powerpc

LIBS= ${L}/sa/libsa.a ${L}/kern/libkern.a ${L}/z/libz.a

.BEGIN: machine powerpc
.NOPATH: machine powerpc
realdepend realall: machine powerpc

machine::
	-rm -f $@
	ln -s ${S}/arch/${MACHINE}/include $@

powerpc::
	-rm -f $@
	ln -s ${S}/arch/powerpc/include $@

vers.c: ${.CURDIR}/../boot/version
	sh ${S}/conf/newvers_stand.sh ${.CURDIR}/../boot/version "prep" ${NEWVERSWHAT}

${PROG}: ${OBJS} machine powerpc
	${LD} -o ${PROG} -s -N -T ${.CURDIR}/../boot/ld.script \
		-Ttext ${RELOC} ${OBJS} ${LIBS}
	${COMPILE.c} -DKERN_OFFSET=${KERN_OFFSET} -DHEAD_SIZE=0x${HEAD_SIZE} \
		${.CURDIR}/../boot/inkernel.c
	${LD} -o ${PROG} -s -N -T ${.CURDIR}/../boot/ld.script \
		-Ttext ${RELOC} ${OBJS} ${LIBS}

.include <bsd.prog.mk>
