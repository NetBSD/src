#	$NetBSD: Makefile,v 1.11.2.6 2002/02/14 19:57:10 he Exp $

S=	${.CURDIR}/../../../..

PROG=		ofwboot
FILES=		${PROG}.elf ${PROG}.xcf
SRCS=		Locore.c ofdev.c hfs.c net.c netif_of.c alloc.c vers.c
XCOFFXTRA=	Xcoffxtra.c
XCOFFXTRAOBJ=	Xcoffxtra.o
CFLAGS+=	-msoft-float
#CFLAGS+=	-DDEBUG -DNETIF_DEBUG
CPPFLAGS+=	-D_STANDALONE -DSUPPORT_DHCP -I${.CURDIR}

.PATH:		${S}/arch/powerpc/powerpc
SRCS+=		ofwmagic.S

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
RELOC=		640000
RELOC_FLATFILE=	600000
COPTS=		-Os -mmultiple

ENTRY=		_start

CLEANFILES+=	vers.c vers.o ${PROG}.elf ${PROG}.el1 ${PROG}.mrg ${PROG}.xcf
CLEANFILES+=	machine powerpc

CPPFLAGS+=	-I. -I${.CURDIR}/../../.. -I${.CURDIR}/../../../..
CPPFLAGS+=	-DRELOC=0x${RELOC} -DRELOC_FLATFILE=0x${RELOC_FLATFILE}
#CPPFLAGS+=	-DXCOFF_GLUE		# for booting PCI Powermacs

CLEANFILES+= ${XCOFFXTRAOBJ} boot.o boot1.o

.BEGIN:
	@[ -h machine ] || ln -s ${S}/arch/${MACHINE}/include machine
	@[ -h powerpc ] || ln -s ${S}/arch/powerpc/include powerpc
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
	sh ${.CURDIR}/../newvers.sh ${.CURDIR}/version ${NEWVERSWHAT}

all realall: ${PROG} ${PROG}.xcf ${PROG}.elf

${PROG}: ${OBJS} boot1.o ${LIBSA} ${LIBZ} ${LIBKERN} 
	${LD} -s -N -Ttext ${RELOC_FLATFILE} -Bstatic -o ${PROG}.el1 \
	    ${OBJS} boot1.o ${LIBSA} ${LIBZ} ${LIBKERN}
	${OBJCOPY} -O binary ${PROG}.el1 ${PROG}

.include <bsd.prog.mk>

${PROG}.elf: ${OBJS} boot.o ${LIBSA} ${LIBZ} ${LIBKERN}
	${LD} -s -N -Ttext ${RELOC} -Bstatic -o ${PROG}.elf \
	    ${OBJS} boot.o ${LIBSA} ${LIBZ} ${LIBKERN}

${PROG}.xcf: ${OBJS} boot.o ${XCOFFXTRAOBJ} ${LIBSA} ${LIBZ} ${LIBKERN} ${FIXCOFFTARG}
	${LD} -s -N -T ${.CURDIR}/../fixcoff/elf32_powerpc_merge.x -e _entry \
	    -Ttext ${RELOC} -Bstatic -o ${PROG}.mrg  ${XCOFFXTRAOBJ} \
	    ${OBJS} boot.o ${LIBSA} ${LIBZ} ${LIBKERN}
	${OBJCOPY} -O aixcoff-rs6000 -R .comment -R .note \
	    ${PROG}.mrg ${PROG}.xcf
	${FIXCOFF} ${PROG}.xcf
