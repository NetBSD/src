#	$NetBSD: Makefile,v 1.4 1998/10/05 02:01:57 sakamoto Exp $

S= ${.CURDIR}/../../../..

BASE= boot
PROG= ${BASE}.pef
NEWVERSWHAT= "BOOT"

SRCS= srt0.s boot.c clock.c com.c conf.c cons.c cpu.c crt.c devopen.c
SRCS+= fd.c filesystem.c io.c monitor.c ns16550.c prf.c tgets.c vreset.c
SRCS+= inkernel.c

CLEANFILES+= vers.c vers.o netbsd.gz ${BASE} elf2pef

CPPFLAGS= -I${.CURDIR}/../../.. -I${S} -I${S}/lib/libsa
CPPFLAGS+= -DDBMONITOR -DRELOC=${RELOC} -DENTRY=${ENTRY}
CPPFLAGS+= -DCONS_VGA
#CPPFLAGS+= -DCONS_SERIAL
#CPPFLAGS+= -DCOMSPEED=9600 -DCOMPORT=0x3F8
CPPFLAGS+= -DUSE_SCAN

AFLAGS= -x assembler-with-cpp -traditional-cpp
NOMAN=
STRIPFLAG=
BINMODE= 444

ENTRY= 0x3100
RELOC= 0x700000


KERNEL?= ${S}/arch/bebox/compile/GENERIC/netbsd

### find out what to use for libkern
KERN_AS=	library
.include "${S}/lib/libkern/Makefile.inc"
LIBKERN=	${KERNLIB}

### find out what to use for libz
Z_AS=		library
.include "${S}/lib/libz/Makefile.inc"
LIBZ=		${ZLIB}

### find out what to use for libsa
SA_AS=		library
SAMISCMAKEFLAGS= SA_USE_CREAD=yes
.include "${S}/lib/libsa/Makefile.inc"
LIBSA=		${SALIB}

${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN} elf2pef
	sh ${.CURDIR}/../newvers.sh ${.CURDIR}/version ${NEWVERSWHAT}
	${COMPILE.c} vers.c
	${LD} -o ${BASE} -s -N -T ld.script -Ttext ${RELOC} ${OBJS} \
		${LIBSA} ${LIBZ} ${LIBKERN} vers.o
.if exists (${KERNEL})
	cat ${KERNEL} | gzip -9 > ${.CURDIR}/netbsd.gz
	${.CURDIR}/elf2pef ${BASE} $@ ${.CURDIR}/netbsd.gz
.else
	${.CURDIR}/elf2pef ${BASE} $@
.endif

elf2pef: elf2pef.c pef.h
	${HOST_CC} -O -DENTRY=${ENTRY} -I${.CURDIR} -o $@ ${.CURDIR}/elf2pef.c

.include <bsd.prog.mk>
