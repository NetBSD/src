#	$NetBSD: Makefile,v 1.9 1999/03/27 07:17:51 simonb Exp $
#	@(#)Makefile	8.3 (Berkeley) 2/16/94

S=	${.CURDIR}/../../../..

PROG=	bootxx
RELOC=	80700000	# Room for an almost 7MB kernel
VERS_O=			# no version info in first stage
MKMAN=	no

SRCS=	start.S alloc.c bootxx.c clear_cache.S memcpy.c memset.c printf.S \
	strcmp.S strcpy.S strlen.S twiddle.c

# from sys/lib/libsa:
SRCS+=	disklabel.c dkcksum.c lseek.c open.c read.c ufs.c
# from sys/lib/libkern:
SRCS+=	bcmp.c
CLEANFILES+=${PROG}.aout ${PROG}.map mkboot ${ALL}
.PATH:  ${.CURDIR}/../lib $S/lib/libsa $S/lib/libkern

BOOTDEFADD+=-DBOOTXX -DRELOC=0x${RELOC} -DUFS_NOCLOSE -DUFS_NOSYMLINK \
	-DLIBSA_USE_MEMCPY -DLIBSA_USE_MEMSET

ALL=	rzboot bootrz

all:	${ALL}

${PROG}.aout: ${PROG}
	elf2aout ${PROG} ${PROG}.aout

mkboot:	${.CURDIR}/mkboot.c
	${CC} ${CPPFLAGS} -I${DESTDIR}/usr/include -o mkboot ${.IMPSRC}

rzboot bootrz: ${PROG}.aout mkboot
	./mkboot ${PROG}.aout rzboot bootrz

proginstall:: bootrz rzboot
	${INSTALL} ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.ALLSRC} ${DESTDIR}${BINDIR}

# .include <bsd.prog.mk>
.include "../Makefile.booters"
