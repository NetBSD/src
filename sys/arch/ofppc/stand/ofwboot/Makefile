#	$NetBSD: Makefile,v 1.27.14.2 2017/12/03 11:36:34 jdolecek Exp $

S!=	cd ${.CURDIR}/../../../.. ; pwd

PROG=		ofwboot
NOMAN=		# defined
SRCS=		ofwstart.S Locore.c boot.c ofdev.c net.c netif_of.c
SRCS+=		mbr.c rdb.c
CFLAGS+=	${${ACTIVE_CC} == "gcc":? -msoft-float :}
CFLAGS+=	-Wno-main -ffreestanding
CFLAGS+=	-Wall -Wmissing-prototypes -Wstrict-prototypes -Wpointer-arith
#CPPFLAGS+=	-g -DALLOC_TRACE -DDEBUG #-DOFW_DEBUG -DNETIF_DEBUG
#CPPFLAGS+=	-DOFWDUMP
CPPFLAGS+=	-D_STANDALONE -DSUPPORT_DHCP -I${.CURDIR}
CPPFLAGS+=	-DHEAP_VARIABLE

.include <bsd.own.mk>
.include <bsd.klinks.mk>

DBG=		-Os

SRCS+=		ofwmagic.S
.PATH:		${S}/arch/powerpc/powerpc

SRCS+=		byteorder.c
.PATH:		${S}/lib/libsa

STRIPFLAG=
BINMODE=	444

# XXX SHOULD NOT NEED TO DEFINE THESE!
LIBCRT0=
LIBCRTI=
LIBC=
LIBCRTBEGIN=
LIBCRTEND=

NEWVERSWHAT=	"OpenFirmware Boot"

# For now...
RELOC=		20000

ENTRY=		_start

CPPFLAGS+=	-I. -I${S} -I${S}/../common/include
CPPFLAGS+=	-DRELOC=0x${RELOC}
CPPFLAGS+=	-DFIRMWORKSBUGS
CPPFLAGS+=	-DCOMPAT_386BSD_MBRPART

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
	${LD} -s -N -T ${.CURDIR}/ldscript -Ttext ${RELOC} -Bstatic \
	    -e ${ENTRY} -o ${PROG}.X ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	mv -f ${PROG}.X ${PROG}

.include "${S}/conf/newvers_stand.mk"

.include <bsd.prog.mk>
