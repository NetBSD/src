#	$NetBSD: Makefile,v 1.1 2001/11/01 22:50:18 thorpej Exp $

S=	${.CURDIR}/../../../..

PROG=		ofwboot
SRCS=		srt0.S Locore.c alloc.c boot.c ofdev.c net.c netif_of.c vers.c
CFLAGS+=	-Wno-main
#CPPFLAGS+=	-DDEBUG -DNETIF_DEBUG
CPPFLAGS+=	-DSUPPORT_DHCP -I${.CURDIR}
DBG=		-Os

MKMAN=		no
STRIPFLAG=
BINMODE=	444

NEWVERSWHAT=	"OpenFirmware Boot"

RELOC=		f0000000

ENTRY=		_start

CLEANFILES+=	vers.c

CPPFLAGS+=	-I. -I${.CURDIR}/../../.. -I${.CURDIR}/../../../..
CPPFLAGS+=	-DRELOC=0x${RELOC}

.BEGIN:
	@[ -h machine ] || ln -s ${S}/arch/${MACHINE}/include machine
	@[ -h arm ] || ln -s ${S}/arch/arm/include arm
.NOPATH: machine arm
CLEANFILES+= machine arm

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
	sh ${S}/conf/newvers_stand.sh ${.CURDIR}/version "dnard" ${NEWVERSWHAT}
	
${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${LD} -s -Ttext ${RELOC} -Bstatic -e ${ENTRY} -o ${.TARGET} \
	    ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}

.include <bsd.prog.mk>
