#	$NetBSD: Makefile,v 1.2.20.2 2017/12/03 11:36:30 jdolecek Exp $

NOMAN=		# defined

.include <bsd.own.mk>

S=		${.CURDIR}/../../../..

BINMODE=	444

PROG=		boot
SRCS=		entry.S
SRCS+=		boot.c bootinfo.c clock.c conf.c cons.c devopen.c prf.c tgets.c
SRCS+=		com.c scif.c wd.c wdc.c

CLEANFILES+=	${PROG}

#MMEYE_CPU=	SH3
MMEYE_CPU=	SH4

.if ${MMEYE_CPU} == "SH3"
AFLAGS+=	-DSH3
CPPFLAGS+=	-DSH3
CPPFLAGS+=	-DCONS_COM
RELOC=		8c800000
.endif
.if ${MMEYE_CPU} == "SH4"
CLEANFILES+=	${PROG}.bin ${PROG}.bin.gz
AFLAGS+=	-DSH4
CPPFLAGS+=	-DSH4
CPPFLAGS+=	-DCONS_SCIF
RELOC=		89000000
.endif


AFLAGS+=	-D_LOCORE -D_KERNEL
CFLAGS+=	-ffreestanding
CFLAGS+=	-Wall -Werror -Wno-main
CFLAGS+=	-Wmissing-prototypes -Wstrict-prototypes -Wpointer-arith
CFLAGS+=	-Wno-pointer-sign
CPPFLAGS+=	-nostdinc -D_STANDALONE -I. -I${.OBJDIR} -I${S}
DBG=		-Os

NETBSD_VERS!=	${HOST_SH} ${S}/conf/osrelease.sh
CPPFLAGS+=	-DNETBSD_VERS='"${NETBSD_VERS}"'

# XXX SHOULD NOT NEED TO DEFINE THESE!
LIBCRT0=
LIBCRTI=
LIBC=
LIBCRTBEGIN=
LIBCRTEND=

STRIPFLAG=

ENTRY=		start


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
	${LD} -N -Ttext ${RELOC} -Bstatic -e ${ENTRY} -o ${PROG} \
	    ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
.if ${MMEYE_CPU} == "SH4"
	${OBJCOPY} -S -O binary ${.TARGET} ${.TARGET}.bin
	TOOL_AWK=${TOOL_AWK} TOOL_HEXDUMP=${TOOL_HEXDUMP} \
	    ${HOST_SH} $(.CURDIR)/deflate.sh ${.TARGET}.bin
.endif

.include "${S}/conf/newvers_stand.mk"

cleandir distclean: .WAIT cleanlibdir

cleanlibdir:
	-rm -rf lib

.include <bsd.prog.mk>
.include <bsd.klinks.mk>
