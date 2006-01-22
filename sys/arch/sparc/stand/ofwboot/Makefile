#	$NetBSD: Makefile,v 1.16 2006/01/22 09:17:14 dsl Exp $

CURDIR=	${.CURDIR}
S=	${CURDIR}/../../../..

#
# Override normal settings
#

WARNS=		0

PROG?=		ofwboot
SRCS=		srt0.s Locore.c boot.c ofdev.c alloc.c net.c netif_of.c vers.c
.PATH:		${S}/arch/sparc64/sparc64

# XXX SHOULD NOT NEED TO DEFINE THESE!
LIBCRT0=
LIBC=
LIBCRTBEGIN=
LIBCRTEND=

COPTS+=		-ffreestanding
CWARNFLAGS+=	-Wno-main
CFLAGS+=	${COPTS} ${CEXTRAFLAGS}
CPPFLAGS+=	-D_STANDALONE -DSUN4U
CPPFLAGS+=	-DBOOT_ELF32 -DBOOT_ELF64
CPPFLAGS+=	-DNETBOOT
CPPFLAGS+=	-DSUPPORT_DHCP
#CPPFLAGS+=	-DNETIF_DEBUG 

LINKS+=		${BINDIR}/ofwboot ${BINDIR}/ofwboot.net

NOMAN=		# defined
STRIPFLAG=
BINMODE=	444

NEWVERSWHAT=	"OpenFirmware Boot"

#
# Elf64 defaults to 1MB
#
# We may get address conflicts with other bootloaders, say
# Sun's ufsboot, so we'll pick a reasonably empty address.
#
RELOC=		800000

ENTRY=		_start

CLEANFILES+=	vers.c sparc64 sparc machine

CPPFLAGS+=	-I${CURDIR}/../../.. -I${CURDIR}/../../../.. -I${CURDIR} -I.
CPPFLAGS+=	-I${CURDIR}/../../../../lib/libsa
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

.NOPATH: machine sparc64 sparc
${SRCS}: machine sparc64 sparc

machine:
	rm -f machine
	ln -s sparc64 machine

sparc64:
	rm -f sparc64
	ln -s ${.CURDIR}/../../../sparc64/include sparc64

sparc:
	rm -f sparc
	ln -s ${.CURDIR}/../../../sparc/include sparc

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib

vers.c: version
	${HOST_SH} ${S}/conf/newvers_stand.sh ${CURDIR}/version "sparc64" ${NEWVERSWHAT}

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

.if ${MACHINE_ARCH} == "sparc64"
AFLAGS+=		-Wa,-Av9a
CEXTRAFLAGS?=		-D_LP64
.else
AFLAGS+=		-Wa,-Av8plusa
.endif

.if CROSS
AFLAGS+=		-x assembler-with-cpp -traditional-cpp -D_LOCORE -D__ELF__
CEXTRAFLAGS?=		-D_LP64
.else
AFLAGS+=		-x assembler-with-cpp -traditional-cpp -D_LOCORE -D__ELF__ ${CEXTRAFLAGS}
.endif

NORMAL_S=	${CC} ${AFLAGS} ${CPPFLAGS} -c $<
srt0.o: srt0.s
	${NORMAL_S}

# Explicit dependency for this.
boot.o: boot.c elfXX_exec.c
