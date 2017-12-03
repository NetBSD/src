#	$NetBSD: Makefile,v 1.12.14.2 2017/12/03 11:36:43 jdolecek Exp $

NOMAN=		# defined

.include <bsd.own.mk>
.include <bsd.klinks.mk>

S=		${.CURDIR}/../../../..

WARNS=		2
PROG=		ofwboot
SRCS=		srt0.S Locore.c alloc.c boot.c ofdev.c net.c netif_of.c
CFLAGS+=	-ffreestanding -Wno-main
#CPPFLAGS+=	-DDEBUG -DNETIF_DEBUG
CPPFLAGS+=	-DSUPPORT_DHCP -I${.CURDIR}
CPPFLAGS+=	-D_STANDALONE
DBG=		-Os

SRCS+=		byteorder.c
.PATH:		${S}/lib/libsa

# XXX SHOULD NOT NEED TO DEFINE THESE!
LIBCRT0=
LIBCRTI=
LIBC=
LIBCRTBEGIN=
LIBCRTEND=

STRIPFLAG=
BINMODE=	444

NEWVERSWHAT=	"OpenFirmware Boot"

RELOC=		f0000000

ENTRY=		_start

CPPFLAGS+=	-nostdinc -I. -I${.CURDIR}/../../.. -I${S}
CPPFLAGS+=	-DRELOC=0x${RELOC}
CFLAGS+=	-fno-unwind-tables
CPUFLAGS=	-mcpu=strongarm

CLEANFILES+= ${PROG}.aout

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

${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${_MKTARGET_LINK}
	${LD} -s -N -Ttext ${RELOC} -Bstatic -e ${ENTRY} -o ${.TARGET}.aout \
	    ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${OBJCOPY} -O binary ${.TARGET}.aout ${.TARGET}

.include "${S}/conf/newvers_stand.mk"

.include <bsd.prog.mk>
