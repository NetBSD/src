#	$NetBSD: Makefile,v 1.6 1999/06/21 21:02:49 wrstuden Exp $

S=	${.CURDIR}/../../../..

PROG=		ofwboot
FILES=		${PROG}.elf
SRCS=		Locore.c boot.c ofdev.c net.c netif_of.c alloc.c
.PATH:		${S}/arch/powerpc/powerpc
SRCS+=		ofwmagic.S
#CFLAGS+=	-DDEBUG -DNETIF_DEBUG
MKMAN=		no
STRIPFLAG=
BINMODE=	444
OBJCOPY?=	objcopy

NEWVERSWHAT=	"OpenFirmware Boot"

# For now...
#RELOC=		20000
RELOC=		6c0000

ENTRY=		_start

CLEANFILES+=	vers.c vers.o ${PROG}.elf

CPPFLAGS+=	-I. -I${.CURDIR}/../../.. -I${.CURDIR}/../../../..
CPPFLAGS+=	-DRELOC=0x${RELOC}
#CPPFLAGS+=	-DXCOFF_GLUE		# for booting PCI Powermacs

CLEANFILES+= ${.OBJDIR}/machine ${.OBJDIR}/powerpc

.BEGIN:
	@[ -h machine ] || ln -s ${S}/arch/${MACHINE}/include machine
	@[ -h powerpc ] || ln -s ${S}/arch/powerpc/include powerpc

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

${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	sh ${.CURDIR}/../newvers.sh ${.CURDIR}/version ${NEWVERSWHAT}
	${COMPILE.c} vers.c
	${LD} -s -N -Ttext ${RELOC} -Bstatic -o ${PROG} \
	    ${OBJS} vers.o ${LIBSA} ${LIBZ} ${LIBKERN}
	mv ${PROG} ${PROG}.elf
	${OBJCOPY} -O binary ${PROG}.elf ${PROG}
#	${OBJCOPY} --input-target=elf32-powerpc \
#	    --output-target=xcoff-powermac ${PROG}.elf ${PROG}.xcf

.include <bsd.prog.mk>
