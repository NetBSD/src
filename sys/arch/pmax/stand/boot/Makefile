#	$NetBSD: Makefile,v 1.1 1999/03/25 12:10:12 simonb Exp $
#	@(#)Makefile	8.3 (Berkeley) 2/16/94

RELOC_BOOT=80710000

S=	${.CURDIR}/../../../..

PROG=	boot
BINDIR=	/
SRCS=	start.S boot.c # bootinfo.c callvec.c conf.c \
NEWVERSWHAT="Secondary Boot"
CLEANFILES+=vers.c vers.o
CLEANFILES+=${PROG}.map
.PATH:	${.CURDIR}/../lib

# XXX These aren't build by libkern at the moment - fix libkern?
SRCS+=	bcmp.c bzero.c
.PATH:	$S/lib/libkern

MKMAN=	no
LDBUG=	-T $S/arch/mips/conf/stand.ldscript

### find out what to use for libsa
SA_AS=		library
.include "${S}/lib/libsa/Makefile.inc"
LIBSA=		${SALIB}
SAMISCMAKEFLAGS= SA_USE_CREAD=yes SA_INCLUDE_NET=no

### find out what to use for libz
Z_AS=		library
.include "${S}/lib/libz/Makefile.inc"
LIBZ=		${ZLIB}

### find out what to use for libkern
KERN_AS=	library
.include "${S}/lib/libkern/Makefile.inc"
LIBKERN=	${KERNLIB}

PMAX_STAND_DIR?= $S/arch/pmax/stand
### find out what to use for libpmax
PMAXDIR= ${PMAX_STAND_DIR}/lib
.include "${PMAXDIR}/Makefile.inc"
LIBPMAX=		${PMAXLIB}


#LIBS=	${LIBSA} ${LIBZ} ${LIBKERN} ${LIBPMAX}
LIBS=	${LIBPMAX} ${LIBSA} ${LIBZ} ${LIBSA} ${LIBKERN} ${LIBPMAX}

${PROG}: ${OBJS} ${LIBS}
	sh ${.CURDIR}/../lib/newvers.sh ${.CURDIR}/version ${NEWVERSWHAT}
	${COMPILE.c} vers.c
	ld -Map ${PROG}.map -N -Ttext ${RELOC_BOOT} ${LDBUG} -e start \
		${OBJS} vers.o ${LIBS} -o ${PROG}

.include <bsd.prog.mk>
