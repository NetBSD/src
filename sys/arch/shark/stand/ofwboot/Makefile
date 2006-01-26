#	$NetBSD: Makefile,v 1.9 2006/01/26 23:26:20 uwe Exp $

S=		${.CURDIR}/../../../..

WARNS=		3
PROG=		ofwboot
SRCS=		srt0.S Locore.c alloc.c boot.c ofdev.c net.c netif_of.c vers.c
CFLAGS+=	-ffreestanding -Wno-main
#CPPFLAGS+=	-DDEBUG -DNETIF_DEBUG
CPPFLAGS+=	-DSUPPORT_DHCP -I${.CURDIR}
CPPFLAGS+=	-D_STANDALONE
DBG=		-Os

SRCS+=		byteorder.c
.PATH:		${S}/lib/libsa

# XXX SHOULD NOT NEED TO DEFINE THESE!
LIBCRT0=
LIBC=
LIBCRTBEGIN=
LIBCRTEND=

NOMAN=		# defined
STRIPFLAG=
BINMODE=	444

NEWVERSWHAT=	"OpenFirmware Boot"

RELOC=		f0000000

ENTRY=		_start

CLEANFILES+=	vers.c

CPPFLAGS+=	-nostdinc -I. -I${.CURDIR}/../../.. -I${S}
CPPFLAGS+=	-DRELOC=0x${RELOC}

.if !make(obj) && !make(clean) && !make(cleandir)
.BEGIN:
	@[ -h machine ] || ln -s ${S}/arch/${MACHINE}/include machine
	@[ -h arm ] || ln -s ${S}/arch/arm/include arm
.endif

.NOPATH: machine arm
CLEANFILES+= machine arm

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

.PHONY: vers.c
vers.c: version
	${HOST_SH} ${S}/conf/newvers_stand.sh ${.CURDIR}/version "shark" ${NEWVERSWHAT}

${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${LD} -s -N -Ttext ${RELOC} -Bstatic -e ${ENTRY} -o ${.TARGET}.aout \
	    ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${OBJCOPY} -O binary ${.TARGET}.aout ${.TARGET}

.include <bsd.prog.mk>
