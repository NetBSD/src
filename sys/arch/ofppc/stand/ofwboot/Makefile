#	$NetBSD: Makefile,v 1.7 1999/10/12 19:40:27 wrstuden Exp $

S=	${.CURDIR}/../../../..

PROG=		ofwboot
SRCS=		Locore.c boot.c ofdev.c net.c netif_of.c alloc.c
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

CLEANFILES+=	vers.c vers.o

CPPFLAGS+=	-I${.CURDIR}/../../.. -I${.CURDIR}/../../../.. -I${.OBJDIR}
CPPFLAGS+=	-DRELOC=0x${RELOC}
CPPFLAGS+=	-DFIRMWORKSBUGS
CPPFLAGS+=	-DPOWERPC_BOOT_AOUT
CPPFLAGS+=	-DPOWERPC_BOOT_ELF
CPPFLAGS+=	-DCOMPAT_386BSD_MBRPART

.BEGIN:
	@[ -h machine ] || ln -s ${S}/arch/${MACHINE}/include machine
	@[ -h powerpc ] || ln -s ${S}/arch/powerpc/include powerpc

CLEANFILES+= ${.OBJDIR}/machine  ${.OBJDIR}/powerpc

${.OBJDIR}/machine::
	-rm -f $@
	ln -s $S/arch/ofppc/include $@

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

${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	sh ${.CURDIR}/../newvers.sh ${.CURDIR}/version ${NEWVERSWHAT}
	${COMPILE.c} vers.c
	${LD} -X -Ttext ${RELOC} -e ${ENTRY} -o ${PROG} \
	    ${OBJS} vers.o ${LIBSA} ${LIBZ} ${LIBKERN}

.include <bsd.prog.mk>
