#	$NetBSD: Makefile,v 1.4 1998/08/23 02:48:28 eeh Exp $

CURDIR=	${.CURDIR}
S=	${CURDIR}/../../../..

#
# Override normal settings
#
#MACHINE=sparc64
#MACHINE_ARCH=sparc64
LD?=	/usr/ccs/bin/ld	
LORDER=lorder 
TSORT=tsort 
NM=nm

PROG=		ofwboot
SRCS=		srt0.s Locore.c boot.c ofdev.c alloc.c #net.c netif_of.c 
.PATH:		${S}/arch/sparc64/sparc64
#SRCS+=		ofwmagic.S

.if CROSS
CFLAGS+=	-DNETIF_DEBUG -D__ELF__ -nostdinc
.else
CFLAGS+=	-DNETIF_DEBUG -D__ELF__ -D_LP64 -dp # -Wa,-Av9a #-nostdinc
.endif

NOMAN=
STRIPFLAG=
BINMODE=	444
OBJCOPY?=	objcopy

NEWVERSWHAT=	"OpenFirmware Boot"

# For now...
.if CROSS
RELOC=		20000
.else
RELOC=		100000
.endif

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

.if CROSS
${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	sh ${CURDIR}/../newvers.sh ${CURDIR}/version ${NEWVERSWHAT}
	${COMPILE.c} vers.c
	${LD} -N -p -Ttext ${RELOC} -e ${ENTRY} >lderr -o ${PROG} \
	    ${OBJS} vers.o -L${SADST} -lsa -L${ZDST} -lz -L${KERNDST} -lkern # solaris linker
	mv ${PROG} ${PROG}.elf
#	${OBJCOPY} --input-target=elf32-sparc \
#	    --output-target=a.out-sunos-big ${PROG}.elf ${PROG}.aout
.else
${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	sh ${CURDIR}/../newvers.sh ${CURDIR}/version ${NEWVERSWHAT}
	${COMPILE.c} vers.c
	${LD} -X -N -Ttext ${RELOC} -e ${ENTRY} -o ${PROG} \
	    ${OBJS} vers.o ${LIBSA} ${LIBZ} ${LIBKERN}	# native linker
	mv ${PROG} ${PROG}.elf
#	${OBJCOPY} --input-target=elf32-sparc \
#	    --output-target=a.out-sunos-big ${PROG}.elf ${PROG}.aout
.endif

.include <bsd.prog.mk>

.if CROSS
AFLAGS=		-x assembler-with-cpp -Wa,-Av9a -traditional-cpp -D_LOCORE -D__ELF__
.else
AFLAGS=		-x assembler-with-cpp -traditional-cpp -D_LOCORE -D__ELF__ -Wa,-Av9 #-Wa,-Av8plusa
.endif
NORMAL_S=	${CC} ${AFLAGS} ${CPPFLAGS} -c $<
srt0.o: srt0.s
	${NORMAL_S}
