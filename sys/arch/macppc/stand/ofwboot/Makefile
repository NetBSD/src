#	$NetBSD: Makefile,v 1.6.8.1 1999/12/27 18:32:44 wrstuden Exp $

S=	${.CURDIR}/../../../..

PROG=		ofwboot
FILES=		${PROG}.elf
SRCS=		Locore.c boot.c ofdev.c net.c netif_of.c alloc.c
XCOFFXTRA=	Xcoffxtra.c
XCOFFXTRAOBJ=	Xcoffxtra.o
.PATH:		${S}/arch/powerpc/powerpc
SRCS+=		ofwmagic.S
#CFLAGS+=	-DDEBUG -DNETIF_DEBUG
MKMAN=		no
STRIPFLAG=
BINMODE=	444
OBJCOPY?=	objcopy

.if !defined(FIXCOFF)
.ifnmake(print-objdir)
FIXCOFFDIR!=	cd ${.CURDIR}/../fixcoff; ${MAKE} print-objdir
FIXCOFF=	${FIXCOFFDIR}/fixcoff
FIXCOFFTARG=	${FIXCOFF}

${FIXCOFF}:
	(cd ${.CURDIR}/../fixcoff; make)

.endif
.else
FIXCOFFTARG=
.endif

NEWVERSWHAT=	"OpenFirmware Boot"

# For now...
#RELOC=		20000
RELOC=		6c0000

ENTRY=		_start

CLEANFILES+=	vers.c vers.o ${PROG}.elf

CPPFLAGS+=	-I. -I${.CURDIR}/../../.. -I${.CURDIR}/../../../..
CPPFLAGS+=	-DRELOC=0x${RELOC}
#CPPFLAGS+=	-DXCOFF_GLUE		# for booting PCI Powermacs

CLEANFILES+= ${.OBJDIR}/machine ${.OBJDIR}/powerpc ${XCOFFXTRAOBJ}

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

${PROG}: ${OBJS} ${XCOFFXTRAOBJ} ${LIBSA} ${LIBZ} ${LIBKERN} ${FIXCOFFTARG}
	sh ${.CURDIR}/../newvers.sh ${.CURDIR}/version ${NEWVERSWHAT}
	${COMPILE.c} vers.c
	${LD} -s -N -Ttext ${RELOC} -Bstatic -o ${PROG}.elf \
	    ${OBJS} vers.o ${LIBSA} ${LIBZ} ${LIBKERN}
	${OBJCOPY} -O binary ${PROG}.elf ${PROG}
	${LD} -s -N -T ${.CURDIR}/../fixcoff/elf32_powerpc_merge.x -e _entry \
	    -Ttext ${RELOC} -Bstatic -o ${PROG}.mrg  ${XCOFFXTRAOBJ} \
	    ${OBJS} vers.o ${LIBSA} ${LIBZ} ${LIBKERN}
	${OBJCOPY} -O aixcoff-rs6000 -R .comment -R .note \
	    ${PROG}.mrg ${PROG}.xcf
	${FIXCOFF} ${PROG}.xcf

.include <bsd.prog.mk>
