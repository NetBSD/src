#	$NetBSD: Makefile,v 1.18 2000/07/25 21:53:55 mycroft Exp $

CURDIR=	${.CURDIR}
S=	${CURDIR}/../../../..

#
# Override normal settings
#

WARNS=		0

PROG=		ofwboot
SRCS=		srt0.s Locore.c boot.c ofdev.c alloc.c net.c netif_of.c vers.c
.PATH:		${S}/arch/sparc64/sparc64

.if CROSS
CEXTRAFLAGS?=	-nostdinc
.else
CEXTRAFLAGS?=	-D_LP64
.endif
CWARNFLAGS+=	-Wno-main
CFLAGS+=	${COPTS} -DNETIF_DEBUG -D__ELF__ ${CEXTRAFLAGS} -D_STANDALONE

MKMAN=		no
STRIPFLAG=
BINMODE=	444
OBJCOPY?=	objcopy

NEWVERSWHAT=	"OpenFirmware Boot"

#
# Elf64 defaults to 1MB
#
# We may get address conflicts with other bootloaders, say
# Sun's ufsboot, so we'll pick a reasonably empty address.
#
RELOC=		800000

ENTRY=		_start

CLEANFILES+=	vers.c

CPPFLAGS+=	-I${CURDIR}/../../.. -I${CURDIR}/../../../.. -I${CURDIR}
CPPFLAGS+=	-DRELOC=0x${RELOC}

#
# XXXXX FIXME
#
CPPFLAGS+=	-DSPARC_BOOT_AOUT
CPPFLAGS+=	-DSPARC_BOOT_ELF
CPPFLAGS+=	-DSPARC_BOOT_UFS
CPPFLAGS+=	-DSPARC_BOOT_NFS
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

.NOPATH: machine
${OBJS}: machine
machine:
	ln -s ../../include machine

vers.c: version
	sh ${S}/conf/newvers_stand.sh ${CURDIR}/version "sparc64" ${NEWVERSWHAT}

.if CROSS
${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${LD} -N -p -Ttext ${RELOC} -e ${ENTRY} >lderr -o ${PROG} \
	    ${OBJS} -L${SADST} -lsa -L${ZDST} -lz -L${KERNDST} -lkern # solaris linker
.else
${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${LD} -X -N -Ttext ${RELOC} -e ${ENTRY} -o ${PROG} \
	    ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}	# native linker
.endif

.include <bsd.prog.mk>

.if CROSS
AFLAGS=		-x assembler-with-cpp -Wa,-Av9a -traditional-cpp -D_LOCORE -D__ELF__
.else
AFLAGS=		-x assembler-with-cpp -traditional-cpp -D_LOCORE -D__ELF__ -Wa,-Av9a ${CEXTRAFLAGS} #-Wa,-Av8plusa
.endif
NORMAL_S=	${CC} ${AFLAGS} ${CPPFLAGS} -c $<
srt0.o: srt0.s
	${NORMAL_S}

# Explicit dependency for this.
boot.o: boot.c elfXX_exec.c
