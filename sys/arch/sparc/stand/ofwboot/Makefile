#	$NetBSD: Makefile,v 1.29.14.2 2017/12/03 11:36:44 jdolecek Exp $

.include <bsd.init.mk>

CURDIR=	${.CURDIR}
S=	${CURDIR}/../../../..

#
# Override normal settings
#

WARNS?=		2
KLINK_MACHINE=	sparc64

PROG?=		ofwboot
SRCS=		srt0.s Locore.c boot.c ofdev.c alloc.c net.c netif_of.c
SRCS+=		bootinfo.c loadfile_machdep.c promlib.c prf.c isfloppy.c
.if ${MACHINE_ARCH} == "sparc64"
SRCS+=		hvcall.S
CPPFLAGS+=	-DSUN4V
.endif
.PATH:		${S}/arch/sparc64/sparc64 ${S}/arch/sparc/stand/common

# XXX SHOULD NOT NEED TO DEFINE THESE!
LIBCRT0=
LIBCRTI=
LIBC=
LIBCRTBEGIN=
LIBCRTEND=

COPTS+=		-ffreestanding -mcpu=v9
CWARNFLAGS+=	-Wno-main
CFLAGS+=	${COPTS}
CPPFLAGS+=	-D_STANDALONE
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
CPPFLAGS+=	-I${CURDIR}/../../../../../common/include
CPPFLAGS+=	-DRELOC=0x${RELOC}

#
# XXXXX FIXME
#
CPPFLAGS+=	-DSPARC_BOOT_AOUT
CPPFLAGS+=	-DSPARC_BOOT_ELF
CPPFLAGS+=	-DSPARC_BOOT_UFS
CPPFLAGS+=	-DSPARC_BOOT_NFS
CPPFLAGS+=	-DSPARC_BOOT_CD9660

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
SAMISCCPPFLAGS+= -DCOMPAT_SOLARIS_UFS
.include "${S}/lib/libsa/Makefile.inc"
LIBSA=		${SALIB}

.include "${S}/conf/newvers_stand.mk"

.include <bsd.own.mk>
.include <bsd.klinks.mk>

.if CROSS
${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${_MKTARGET_LINK}
	${LD} -N -p -Ttext ${RELOC} -e ${ENTRY} >lderr -o ${PROG} \
	    ${OBJS} -L${SADST} -lsa -L${ZDST} -lz -L${KERNDST} -lkern # solaris linker
.else
${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${_MKTARGET_LINK}
	${LD} -X -N -S -Ttext ${RELOC} -e ${ENTRY} -o ${PROG} \
	    ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}	# native linker
.endif

.include <bsd.prog.mk>

CPUFLAGS+=		-mcpu=v9
AFLAGS+=		-Wa,-Av9a
AFLAGS+=		-x assembler-with-cpp -D_LOCORE -D__ELF__

NORMAL_S=	${CC} ${AFLAGS} ${AFLAGS.${<:T}}  ${CPPFLAGS} -c $<
srt0.o: srt0.s
	${NORMAL_S}

hvcall.o: hvcall.S
	${NORMAL_S}


# Explicit dependency for this.
boot.o: boot.c
