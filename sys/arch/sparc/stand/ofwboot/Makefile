#	$NetBSD: Makefile,v 1.15.2.1 2006/02/01 14:51:37 yamt Exp $

CURDIR=	${.CURDIR}
S=	${CURDIR}/../../../..

#
# Override normal settings
#

WARNS=		0

PROG?=		ofwboot
SRCS=		srt0.s Locore.c boot.c ofdev.c alloc.c net.c netif_of.c vers.c
SRCS+=		bootinfo.c loadfile_machdep.c promlib.c prf.c
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
CPPFLAGS+=	-DBOOT_ELF32 -DBOOT_ELF64 -DBOOT_AOUT
CPPFLAGS+=	-DNETBOOT
CPPFLAGS+=	-DSUPPORT_DHCP
#CPPFLAGS+=	-DNETIF_DEBUG 
#CPPFLAGS+=	-D_DEBUG

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
RELOC=		100000

ENTRY=		_start

CPPFLAGS+=	-I${CURDIR}/../../.. -I${CURDIR}/../../../.. -I${CURDIR} -I.
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
SAMISCMAKEFLAGS= SA_USE_LOADFILE=yes SA_USE_CREAD=yes
.include "${S}/lib/libsa/Makefile.inc"
LIBSA=		${SALIB}

.if !make(obj) && !make(clean) && !make(cleandir)
.BEGIN:
	@([ -h machine ] || ln -s ${S}/arch/sparc64/include machine)
	@([ -h sparc64 ] || ln -s ${S}/arch/sparc64/include sparc64)
	@([ -h sparc ] || ln -s ${S}/arch/sparc/include sparc)
.NOPATH: machine sparc64 sparc
.endif
CLEANFILES+= machine sparc64 sparc

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib

vers.c: version
	${HOST_SH} ${S}/conf/newvers_stand.sh ${CURDIR}/version "sparc64" ${NEWVERSWHAT}

CLEANFILES+= vers.c

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
boot.o: boot.c
