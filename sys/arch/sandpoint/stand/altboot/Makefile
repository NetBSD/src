#	$NetBSD: Makefile,v 1.16 2014/08/10 17:46:54 joerg Exp $

S=		${.CURDIR}/../../../..

PROG=		altboot
FILES+=		${PROG}.bin ${PROG}.img
NOMAN=		# defined
SRCS=		entry.S main.c brdsetup.c pci.c devopen.c dev_net.c memfs.c
SRCS+=		nif.c fxp.c tlp.c rge.c skg.c stg.c
SRCS+=		dsk.c pciide.c siisata.c
SRCS+=		exception.c vers.c
CLEANFILES+=	vers.c ${PROG} ${PROG}.bin ${PROG}.img
CFLAGS+=	-Wall -Wno-main -ffreestanding
CFLAGS+=	${${ACTIVE_CC} == "gcc":? -msoft-float :}
CFLAGS+=	-Wmissing-prototypes -Wstrict-prototypes -Wpointer-arith
CPPFLAGS+=	-D_STANDALONE -DSUPPORT_DHCP -DLIBSA_PRINTF_WIDTH_SUPPORT
#CPPFLAGS+=	-DCONSNAME=\"com\" -DCONSPORT=0x3f8 -DCONSSPEED=115200
#CPPFLAGS+=	-DCONSNAME=\"eumb\" -DCONSPORT=0x4600 -DCONSSPEED=57600
CPPFLAGS+=	-nostdinc -I. -I${.OBJDIR} -I${S}
DBG=		-Os

.include <bsd.own.mk>
.include <bsd.klinks.mk>

# XXX SHOULD NOT NEED TO DEFINE THESE!
LIBCRT0=
LIBCRTI=
LIBC=
LIBCRTBEGIN=
LIBCRTEND=

STRIPFLAG=
BINMODE=	444

RELOC=		1000000
ENTRY=		_start

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
	${HOST_SH} ${S}/conf/newvers_stand.sh -K \
	    ${${MKREPRO} == "yes" :?:-D} ${.CURDIR}/version "sandpoint"

${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${_MKTARGET_LINK}
	${LD} -N -Ttext ${RELOC} -Bstatic -e ${ENTRY} -o ${PROG} \
	    ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${OBJCOPY} -S -O binary ${PROG} ${PROG}.bin
	${TOOL_MKUBOOTIMAGE} -A powerpc -T kernel -C none -O linux \
	    -a 0x${RELOC} -n ${PROG} ${PROG}.bin ${PROG}.img

.include <bsd.prog.mk>
