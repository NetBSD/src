#	$NetBSD: Makefile,v 1.8 1999/03/25 12:26:32 simonb Exp $
#	@(#)Makefile	8.3 (Berkeley) 2/16/94

RELOC_BOOTXX=80700000	# Room for an almost 7MB kernel

S=	${.CURDIR}/../../../..

PROG=	bootxx
SRCS=	start.S alloc.c bootxx.c clear_cache.S memcpy.c memset.c printf.S \
	strcmp.S strcpy.S strlen.S twiddle.c

# from sys/lib/libsa:
SRCS+=	disklabel.c dkcksum.c lseek.c open.c read.c ufs.c
# from sys/lib/libkern:
SRCS+=	bcmp.c
CLEANFILES+=${PROG}.elf ${PROG}.map ${ALL}
.PATH:  ${.CURDIR}/../lib $S/lib/libsa $S/lib/libkern

BOOTDEFADD+=-DBOOTXX -DRELOC=0x${RELOC_BOOTXX} -DUFS_NOCLOSE -DUFS_NOSYMLINK \
	-DLIBSA_USE_MEMCPY -DLIBSA_USE_MEMSET

MKMAN=	no
LDBUG=	-T $S/arch/mips/conf/stand.ldscript

ALL=	bootxx mkboot rzboot bootrz

all:	${ALL}

PMAX_STAND_DIR?= $S/arch/pmax/stand
### find out what to use for libpmax
PMAXDIR= ${PMAX_STAND_DIR}/lib
.include "${PMAXDIR}/Makefile.inc"
LIBPMAX=		${PMAXLIB}

${PROG}: ${OBJS} ${LIBPMAX}
	ld -Map ${PROG}.map -N -x -Ttext ${RELOC_BOOTXX} ${LDBUG} \
		-e start ${OBJS} ${LIBPMAX} -o ${PROG}.elf
	elf2aout ${PROG}.elf ${PROG}

mkboot:	${.CURDIR}/mkboot.c
	${CC} ${CPPFLAGS} -I${DESTDIR}/usr/include -o mkboot ${.IMPSRC}

rzboot bootrz:	mkboot ${PROG}
	./mkboot ${PROG} rzboot bootrz

proginstall:: bootrz rzboot
	${INSTALL} ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.ALLSRC} ${DESTDIR}${BINDIR}

.include <bsd.prog.mk>
