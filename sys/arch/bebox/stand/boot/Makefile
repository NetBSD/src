#	$NetBSD: Makefile,v 1.7 1999/06/24 01:10:31 sakamoto Exp $

S= ${.CURDIR}/../../../..

BASE= boot
BOOTPROG= ${BASE}.pef
NEWVERSWHAT= "BOOT"

ASRCS+= srt0.s
CSRCS+= boot.c clock.c com.c conf.c cons.c cpu.c devopen.c
CSRCS+= fd.c filesystem.c inkernel.c io.c tgets.c prf.c monitor.c
CSRCS+= kbd.c ns16550.c vreset.c vga.c video.c

CLEANFILES+= vers.c vers.o netbsd.gz ${BASE} ${BOOTPROG} elf2pef

CPPFLAGS= -I${.CURDIR} -I${.CURDIR}/../../.. -I${S} -I${S}/lib/libsa
CPPFLAGS+= -D_STANDALONE -DDBMONITOR -DRELOC=${RELOC} -DENTRY=${ENTRY}
#CPPFLAGS+= -DUSE_SCAN
#CPPFLAGS+= -DCONS_BE
#CPPFLAGS+= -DCONS_VGA
CPPFLAGS+= -DCONS_SERIAL -DCOMSPEED=9600 -DCOMPORT=0x3F8

AOBJS=	${ASRCS:.s=.o}
COBJS=	${CSRCS:.c=.o}
OBJS=	${AOBJS} ${COBJS}
AFLAGS= -x assembler-with-cpp -traditional-cpp
MKMAN=	no
STRIPFLAG=
BINMODE= 444

ENTRY= 0x3100
RELOC= 0x700000

CLEANFILES+= ${.OBJDIR}/machine ${.OBJDIR}/powerpc

.BEGIN:
	@[ -h machine ] || ln -s ${S}/arch/${MACHINE}/include machine
	@[ -h powerpc ] || ln -s ${S}/arch/powerpc/include powerpc

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

all: ${BOOTPROG}

${BOOTPROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN} elf2pef
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
	${HOST_LINK.c} -I/usr/include -I${.CURDIR} \
		-DENTRY=${ENTRY} -o ${.TARGET} ${.IMPSRC}

.include <bsd.prog.mk>
