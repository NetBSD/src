#	$NetBSD: Makefile,v 1.1.2.1 2007/06/02 08:44:36 nisimura Exp $

S=		${.CURDIR}/../../../..

PROG=		netboot
SRCS=		entry.S main.c pci.c devopen.c dev_net.c nif_fxp.c fxp.c \
		printf.c vers.c
CLEANFILES+=	vers.c ${PROG} ${PROG}.bin
CFLAGS+=	-Wall -Wno-main -ffreestanding -msoft-float -mmultiple
CPPFLAGS+=	-D_STANDALONE -DSUPPORT_DHCP
CPPFLAGS+=	-DCONSNAME=\"com\" -DCONSPORT=0x3f8 -DCONSSPEED=115200
#CPPFLAGS+=	-DCONSNAME=\"eumb\" -DCONSPORT=0x4600 -DCONSSPEED=115200
#CPPFLAGS+=	-DDEBUG
CPPFLAGS+=	-nostdinc -I. -I${S}
DBG=		-Os

# XXX SHOULD NOT NEED TO DEFINE THESE!
LIBCRT0=
LIBC=
LIBCRTBEGIN=
LIBCRTEND=

NOMAN=		# defined
STRIPFLAG=
BINMODE=	444

RELOC=		1000000
ENTRY=		_start

.BEGIN:
	@[ -h machine ] || ln -s ../../../${MACHINE}/include machine
	@[ -h powerpc ] || ln -s ../../../powerpc/include powerpc
.NOPATH: machine arm
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
	${HOST_SH} ./newvers.sh ${.CURDIR}/version

${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${LD} -N -Ttext ${RELOC} -Bstatic -e ${ENTRY} -o ${PROG} \
	    ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${OBJCOPY} -S -O binary ${.TARGET} ${.TARGET}.bin

.include <bsd.prog.mk>
