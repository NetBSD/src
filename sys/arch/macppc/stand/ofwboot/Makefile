#	$NetBSD: Makefile,v 1.4.2.1 2000/07/13 21:00:43 he Exp $

S=	${.CURDIR}/../../../..

PROG=		ofwboot
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

CPPFLAGS+=	-I${.CURDIR}/../../.. -I${.CURDIR}/../../../..
CPPFLAGS+=	-DRELOC=0x${RELOC}
CPPFLAGS+=	-DPOWERPC_BOOT_AOUT
CPPFLAGS+=	-DPOWERPC_BOOT_ELF
#CPPFLAGS+=	-DXCOFF_GLUE		# for booting PCI Powermacs

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
	sh ${.CURDIR}/../newvers.sh ${.CURDIR}/version ${NEWVERSWHAT}
	${COMPILE.c} vers.c
	${LD} -s -N -Ttext ${RELOC} -Bstatic -o ${PROG} \
	    ${OBJS} vers.o ${LIBSA} ${LIBZ} ${LIBKERN}
	mv ${PROG} ${PROG}.elf
	tail -c +117 ${PROG}.elf > ${PROG}
#	${OBJCOPY} --input-target=elf32-powerpc \
#	    --output-target=xcoff-powermac ${PROG}.elf ${PROG}.xcf

.if !defined(RELEASEDIR)
release:
	@echo setenv RELEASEDIR before doing that!
	@false
.else
release: ${PROG}.elf
	${INSTALL} -c ${PROG}.elf ${RELEASEDIR}/binary/kernel/${PROG}.elf
.endif
.include <bsd.prog.mk>
