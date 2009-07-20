#	$NetBSD: Makefile,v 1.15 2009/07/20 11:43:08 nisimura Exp $

S=		${.CURDIR}/../../../..

PROG=		netboot
SRCS=		entry.S main.c brdsetup.c pci.c devopen.c dev_net.c nif.c \
		fxp.c tlp.c rge.c printf.c
CLEANFILES+=	vers.c vers.o ${PROG} ${PROG}.bin
CFLAGS+=	-Wall -Wno-main -ffreestanding -msoft-float -mmultiple
CFLAGS+=	-Wmissing-prototypes -Wstrict-prototypes -Wpointer-arith
CPPFLAGS+=	-D_STANDALONE -DSUPPORT_DHCP
CPPFLAGS+=	-DCONSNAME=\"com\" -DCONSPORT=0x3f8 -DCONSSPEED=115200
#CPPFLAGS+=	-DCONSNAME=\"eumb\" -DCONSPORT=0x4600 -DCONSSPEED=57600
#CPPFLAGS+=	-DTICKS_PER_SEC=(133333333/4)
#CPPFLAGS+=	-DSTART_DDB_SESSION
CPPFLAGS+=	-nostdinc -I. -I${.OBJDIR} -I${S}
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


.if !make(obj) && !make(clean) && !make(cleandir)
.BEGIN:
	@[ -h machine ] || ln -s ${S}/arch/${MACHINE}/include machine
	@[ -h powerpc ] || ln -s ${S}/arch/powerpc/include powerpc
.NOPATH: machine powerpc
.endif
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

${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${HOST_SH} ${.CURDIR}/newvers.sh ${.CURDIR}/version
	${CC} -c vers.c
	${LD} -N -Ttext ${RELOC} -Bstatic -e ${ENTRY} -o ${PROG} \
	    ${OBJS} vers.o ${LIBSA} ${LIBZ} ${LIBKERN}
	${OBJCOPY} -S -O binary ${.TARGET} ${.TARGET}.bin

.include <bsd.prog.mk>

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib
