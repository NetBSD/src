#	$NetBSD: Makefile,v 1.16 2000/07/25 06:26:18 tsubai Exp $

S=	${.CURDIR}/../../../..

PROG=		ofwboot
FILES=		${PROG}.elf ${PROG}.xcf
SRCS=		Locore.c boot.c ofdev.c net.c netif_of.c vers.c
XCOFFXTRA=	Xcoffxtra.c
XCOFFXTRAOBJ=	Xcoffxtra.o
CFLAGS+=	-msoft-float
#CFLAGS+=	-DDEBUG -DNETIF_DEBUG
CPPFLAGS+=	-D_STANDALONE -DSUPPORT_DHCP
DBG=		-Os -mmultiple

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
RELOC=		6c0000

ENTRY=		_start

CLEANFILES+=	vers.c ${PROG}.elf ${PROG}.mrg ${PROG}.xcf

CPPFLAGS+=	-I. -I${.CURDIR}/../../.. -I${.CURDIR}/../../../..
CPPFLAGS+=	-DRELOC=0x${RELOC}
#CPPFLAGS+=	-DXCOFF_GLUE		# for booting PCI Powermacs

CLEANFILES+= ${XCOFFXTRAOBJ}

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
	sh ${S}/conf/newvers_stand.sh ${.CURDIR}/version "macppc" ${NEWVERSWHAT}

${PROG}: ${OBJS} ${XCOFFXTRAOBJ} ${LIBSA} ${LIBZ} ${LIBKERN} ${FIXCOFFTARG}
	${LD} -s -N -Ttext ${RELOC} -Bstatic -o ${PROG}.elf \
	    ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${OBJCOPY} -O binary ${PROG}.elf ${PROG}
	${LD} -s -N -T ${.CURDIR}/../fixcoff/elf32_powerpc_merge.x -e _entry \
	    -Ttext ${RELOC} -Bstatic -o ${PROG}.mrg  ${XCOFFXTRAOBJ} \
	    ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${OBJCOPY} -O aixcoff-rs6000 -R .comment -R .note \
	    ${PROG}.mrg ${PROG}.xcf
	${FIXCOFF} ${PROG}.xcf

.include <bsd.prog.mk>
