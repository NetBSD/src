# $NetBSD: Makefile,v 1.18 1997/05/09 07:50:26 mycroft Exp $

S=	${.CURDIR}/../../../..

.PATH: ${.CURDIR}/../common

PROG = boot

SRCS = start.S boot.c loadfile.c disk.c conf.c prom.c prom_disp.S OSFpal.c
SRCS+= devopen.c filesystem.c prom_swpal.S
NOMAN=
STRIPFLAG=
BINMODE= 444

BOOT_RELOC = ${SECONDARY_LOAD_ADDRESS}
HEADERSIZE_PROG = headersize

CLEANFILES+= vers.c vers.o ${PROG}.sym ${PROG}.nosym ${HEADERSIZE_PROG}

AFLAGS += -DASSEMBLER
CPPFLAGS += -I${.CURDIR}/../.. -I${.CURDIR}/../../../.. \
	-DALPHA_BOOT_ECOFF -DALPHA_BOOT_ELF
CFLAGS = -Werror -mno-fp-regs -g

${PROG}.nosym: ${PROG}.sym
	cp ${PROG}.sym ${PROG}.nosym
	strip ${PROG}.nosym

${PROG}: ${PROG}.nosym ${HEADERSIZE_PROG}
	dd if=${PROG}.nosym of=${PROG} \
	    bs=`./${HEADERSIZE_PROG} ${BOOT_RELOC} ${PROG}.nosym` skip=1

.include <bsd.prog.mk>

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

${PROG}.sym: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	sh ${.CURDIR}/newvers.sh ${.CURDIR}/version
	${COMPILE.c} vers.c
	${LD} -Ttext ${BOOT_RELOC} -N -e start -o ${PROG}.sym \
	    ${OBJS} vers.o ${LIBSA} ${LIBZ} ${LIBSA} ${LIBKERN}
	size ${PROG}.sym
