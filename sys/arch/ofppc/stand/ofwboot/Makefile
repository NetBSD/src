#	$NetBSD: Makefile,v 1.9 2000/08/07 22:38:11 jdolecek Exp $

S=	${.CURDIR}/../../../..

PROG=		ofwboot
SRCS=		Locore.c boot.c ofdev.c net.c netif_of.c alloc.c vers.c
.PATH:		${S}/arch/ofppc/ofppc
SRCS+=		ofwmagic.S
.PATH:		${S}/arch/powerpc/powerpc
#CFLAGS+=	-DDEBUG -DNETIF_DEBUG
MKMAN=		no
STRIPFLAG=
BINMODE=	444

NEWVERSWHAT=	"OpenFirmware Boot"

# For now...
RELOC=		20000

ENTRY=		_start

CLEANFILES+=	vers.c

CPPFLAGS+=	-I${.CURDIR}/../../.. -I${.CURDIR}/../../../.. -I${.OBJDIR}
CPPFLAGS+=	-DRELOC=0x${RELOC}
CPPFLAGS+=	-DFIRMWORKSBUGS
CPPFLAGS+=	-DPOWERPC_BOOT_AOUT
CPPFLAGS+=	-DPOWERPC_BOOT_ELF
CPPFLAGS+=	-DCOMPAT_386BSD_MBRPART

.BEGIN:
	@[ -h machine ] || ln -s ${S}/arch/${MACHINE}/include machine
	@[ -h powerpc ] || ln -s ${S}/arch/powerpc/include powerpc
.NOPATH: machine powerpc
CLEANFILES+= machine powerpc

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

.PHONY: vers.c
vers.c: version
	sh ${S}/conf/newvers_stand.sh ${.CURDIR}/version "ofppc" ${NEWVERSWHAT}
	
${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${LD} -X -Ttext ${RELOC} -e ${ENTRY} -o ${PROG} \
	    ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}

.include <bsd.prog.mk>
