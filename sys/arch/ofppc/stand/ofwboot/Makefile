#	$NetBSD: Makefile,v 1.18.32.1 2007/07/11 20:01:11 mjf Exp $

S!=	cd ${.CURDIR}/../../../.. ; pwd

PROG=		ofwboot
SRCS=		Locore.c alloc.c boot.c ofdev.c net.c netif_of.c vers.c
CFLAGS+=	-msoft-float -Wno-main -ffreestanding
#CPPFLAGS+=	-DDEBUG -DNETIF_DEBUG
CPPFLAGS+=	-D_STANDALONE -DSUPPORT_DHCP -I${.CURDIR}
DBG=		-Os -mmultiple

SRCS+=		ofwmagic.S
.PATH:		${S}/arch/powerpc/powerpc

SRCS+=		byteorder.c
.PATH:		${S}/lib/libsa

NOMAN=		# defined
STRIPFLAG=
BINMODE=	444

# XXX SHOULD NOT NEED TO DEFINE THESE!
LIBCRT0=
LIBC=
LIBCRTBEGIN=
LIBCRTEND=

NEWVERSWHAT=	"OpenFirmware Boot"

# For now...
RELOC=		20000

ENTRY=		_start

CLEANFILES+=	vers.c

CPPFLAGS+=	-I. -I${S} -I${S}/../common/include
CPPFLAGS+=	-DRELOC=0x${RELOC}
CPPFLAGS+=	-DFIRMWORKSBUGS
CPPFLAGS+=	-DCOMPAT_386BSD_MBRPART

.if !make(obj) && !make(clean) && !make(cleandir)
.BEGIN:
	@[ -h machine ] || ln -s ${S}/arch/${MACHINE}/include machine
	@[ -h powerpc ] || ln -s ${S}/arch/powerpc/include powerpc
.endif
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
SAMISCMAKEFLAGS= SA_USE_CREAD=yes SA_USE_LOADFILE=yes
.include "${S}/lib/libsa/Makefile.inc"
LIBSA=		${SALIB}

.PHONY: vers.c
vers.c: version
	${HOST_SH} ${S}/conf/newvers_stand.sh ${.CURDIR}/version "ofppc" ${NEWVERSWHAT}

${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${LD} -s -N -Ttext ${RELOC} -Bstatic -e ${ENTRY} -o ${PROG}.X \
	    ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	mv -f ${PROG}.X ${PROG}

.include <bsd.prog.mk>
