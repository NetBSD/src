#	$NetBSD: Makefile,v 1.1.1.1 1998/06/20 04:58:53 eeh Exp $

CURDIR=	${.CURDIR}
S=	${CURDIR}/../../../..

#
# Override normal settings
#
#MACHINE=sparc64
#MACHINE_ARCH=sparc64
LD?=	/usr/ccs/bin/ld	

PROG=		ofwboot
SRCS=		srt0.s Locore.c boot.c ofdev.c alloc.c #net.c netif_of.c 
.PATH:		${S}/arch/sparc64/sparc64
#SRCS+=		ofwmagic.S
CFLAGS+=	-DDEBUG -DNETIF_DEBUG
NOMAN=
STRIPFLAG=
BINMODE=	444
OBJCOPY?=	objcopy

NEWVERSWHAT=	"OpenFirmware Boot"

# For now...
RELOC=		20000

ENTRY=		_start

CLEANFILES+=	vers.c vers.o

CPPFLAGS+=	-I${CURDIR}/../../.. -I${CURDIR}/../../../.. -I${CURDIR}
CPPFLAGS+=	-DRELOC=0x${RELOC}

#
# XXXXX FIXME
#
CPPFLAGS+=	-DSPARC_BOOT_AOUT
CPPFLAGS+=	-DSPARC_BOOT_ELF
CPPFLAGS+=	-DSPARC_BOOT_UFS
#CPPFLAGS+=	-DSPARC_BOOT_HSFS

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

${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	sh ${CURDIR}/../newvers.sh ${CURDIR}/version ${NEWVERSWHAT}
	${COMPILE.c} vers.c
#	${LD} -X -Ttext ${RELOC} -e ${ENTRY} -o ${PROG} \
#	    ${OBJS} vers.o ${LIBSA} ${LIBZ} ${LIBKERN}	# native linker
	${LD} -N -p -Ttext ${RELOC} -e ${ENTRY} >lderr -o ${PROG} \
	    ${OBJS} vers.o -L${SADST} -lsa -L${ZDST} -lz -L${KERNDST} -lkern
	mv ${PROG} ${PROG}.elf
#	${OBJCOPY} --input-target=elf32-sparc \
#	    --output-target=a.out-sunos-big ${PROG}.elf ${PROG}.aout

.include <bsd.prog.mk>

AFLAGS=		-x assembler-with-cpp -Wa,-Av9a -traditional-cpp -D_LOCORE
NORMAL_S=	${CC} ${AFLAGS} ${CPPFLAGS} -c $<
srt0.o: srt0.s
	${NORMAL_S}
