#	$NetBSD: Makefile,v 1.29.14.1 2014/08/20 00:03:25 tls Exp $

CURDIR=	${.CURDIR}
S=	${CURDIR}/../../../..

#
# Override normal settings
#

WARNS?=		2
KLINK_MACHINE=	sparc64

PROG?=		ofwboot
SRCS=		srt0.s Locore.c boot.c ofdev.c alloc.c net.c netif_of.c vers.c
SRCS+=		bootinfo.c loadfile_machdep.c promlib.c prf.c isfloppy.c
.if ${MACHINE_ARCH} == "sparc64"
SRCS+=		hvcall.S
.endif
.PATH:		${S}/arch/sparc64/sparc64 ${S}/arch/sparc/stand/common

# XXX SHOULD NOT NEED TO DEFINE THESE!
LIBCRT0=
LIBCRTI=
LIBC=
LIBCRTBEGIN=
LIBCRTEND=

COPTS+=		-ffreestanding
CWARNFLAGS+=	-Wno-main
CFLAGS+=	${COPTS} ${CEXTRAFLAGS}
CPPFLAGS+=	-D_STANDALONE -DSUN4U
.if ${MACHINE_ARCH} == "sparc64"
CPPFLAGS+=	-DSUN4V
.endif
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

.include <bsd.own.mk>
.include <bsd.klinks.mk>

vers.c: version
	${HOST_SH} ${S}/conf/newvers_stand.sh ${${MKREPRO} == "yes" :?:-D} \
	    ${CURDIR}/version "sparc64" ${NEWVERSWHAT}

CLEANFILES+= vers.c

.if CROSS
${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${_MKTARGET_LINK}
	${LD} -N -p -Ttext ${RELOC} -e ${ENTRY} >lderr -o ${PROG} \
	    ${OBJS} -L${SADST} -lsa -L${ZDST} -lz -L${KERNDST} -lkern # solaris linker
.else
${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${_MKTARGET_LINK}
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
AFLAGS+=		-x assembler-with-cpp -D_LOCORE -D__ELF__
CEXTRAFLAGS?=		-D_LP64
.else
AFLAGS+=		-x assembler-with-cpp -D_LOCORE -D__ELF__ ${CEXTRAFLAGS}
.endif

NORMAL_S=	${CC} ${AFLAGS} ${CPPFLAGS} -c $<
srt0.o: srt0.s
	${NORMAL_S}

hvcall.o: hvcall.S
	${NORMAL_S}


# Explicit dependency for this.
boot.o: boot.c
