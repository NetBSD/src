#	$NetBSD: Makefile,v 1.22.38.1 2007/05/02 17:50:41 heinz Exp $

S= ${.CURDIR}/../../../..

PROG= boot
NEWVERSWHAT= "BOOT"

BINMODE?=       444

# XXX SHOULD NOT NEED TO DEFINE THESE!
LIBCRT0=
LIBC=
LIBCRTBEGIN=
LIBCRTEND=

realall: ${PROG}

ASRCS+= srt0.s
CSRCS+= boot.c clock.c com.c conf.c cons.c cpu.c devopen.c
CSRCS+= fd.c filesystem.c inkernel.c io.c tgets.c prf.c monitor.c
CSRCS+= kbd.c ns16550.c vreset.c vga.c video.c

SRCS=	${ASRCS} ${CSRCS}

CLEANFILES+= vers.c vers.o

COPTS+= -ffreestanding
CPPFLAGS= -I. -I${.CURDIR} -I${.CURDIR}/../elf2pef -I${.CURDIR}/../../..
CPPFLAGS+= -I${S} -I${S}/lib/libsa
CPPFLAGS+= -D_STANDALONE -DDBMONITOR -DRELOC=${RELOC}
CPPFLAGS+= -DUSE_SCAN
#CPPFLAGS+= -DCONS_BE
#CPPFLAGS+= -DCONS_VGA
CPPFLAGS+= -DCONS_SERIAL -DCOMSPEED=9600 -DCOMPORT=0x3F8

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
	${HOST_SH} ${S}/conf/newvers_stand.sh ${.CURDIR}/version "bebox" \
		${NEWVERSWHAT}

${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN} vers.o
	${LD} -o ${PROG} -s -N -T ${.CURDIR}/ld.script -Ttext ${RELOC} ${OBJS} \
		${LIBSA} ${LIBZ} ${LIBKERN} vers.o

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib

.include <bsd.prog.mk>
