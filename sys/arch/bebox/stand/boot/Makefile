#	$NetBSD: Makefile,v 1.16.10.3 2004/09/21 13:14:26 skrll Exp $

S= ${.CURDIR}/../../../..

BOOTPROG= boot
NEWVERSWHAT= "BOOT"

ASRCS+= srt0.s
CSRCS+= boot.c clock.c com.c conf.c cons.c cpu.c devopen.c
CSRCS+= fd.c filesystem.c inkernel.c io.c tgets.c prf.c monitor.c
CSRCS+= kbd.c ns16550.c vreset.c vga.c video.c

CLEANFILES+= vers.c vers.o ${BOOTPROG}

COPTS+= -ffreestanding
CPPFLAGS= -I. -I${.CURDIR} -I${.CURDIR}/../elf2pef -I${.CURDIR}/../../..
CPPFLAGS+= -I${S} -I${S}/lib/libsa
CPPFLAGS+= -D_STANDALONE -DDBMONITOR -DRELOC=${RELOC}
CPPFLAGS+= -DUSE_SCAN
#CPPFLAGS+= -DCONS_BE
CPPFLAGS+= -DCONS_VGA
#CPPFLAGS+= -DCONS_SERIAL -DCOMSPEED=9600 -DCOMPORT=0x3F8

AOBJS=	${ASRCS:.s=.o}
COBJS=	${CSRCS:.c=.o}
OBJS=	${AOBJS} ${COBJS}
CFLAGS= -Wno-main
AFLAGS= -x assembler-with-cpp -traditional-cpp
NOMAN=	# defined
STRIPFLAG=
BINMODE= 444

RELOC= 0x700000

.if !make(obj) && !make(clean) && !make(cleandir)
.BEGIN:
	@[ -h machine ] || ln -s ${S}/arch/${MACHINE}/include machine
	@[ -h powerpc ] || ln -s ${S}/arch/powerpc/include powerpc
.endif
.NOPATH: machine powerpc
CLEANFILES+= machine powerpc

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
SAMISCMAKEFLAGS= SA_USE_CREAD=yes SA_USE_LOADFILE=yes
.include "${S}/lib/libsa/Makefile.inc"
LIBSA=		${SALIB}

.PHONY: vers.c
vers.c: version
	${HOST_SH} ${S}/conf/newvers_stand.sh ${.CURDIR}/version "bebox" ${NEWVERSWHAT}

realall: ${BOOTPROG}

${BOOTPROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN} vers.o
	${LD} -o ${BOOTPROG} -s -N -T ${.CURDIR}/ld.script -Ttext ${RELOC} ${OBJS} \
		${LIBSA} ${LIBZ} ${LIBKERN} vers.o

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib

.include <bsd.prog.mk>
