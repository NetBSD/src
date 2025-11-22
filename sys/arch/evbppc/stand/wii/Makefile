# $NetBSD: Makefile,v 1.1.2.3 2025/11/22 13:26:42 martin Exp $

S=		${.CURDIR}/../../../..
POWERPC=	${S}/arch/powerpc

PROG?=		ppcboot.elf
NEWVERSWHAT=	"Wii MINI boot"

SRCS+=		start.S
SRCS+=		boot.c
SRCS+=		console.c
SRCS+=		conf.c
SRCS+=		devopen.c
SRCS+=		gecko.c
SRCS+=		gpio.c
SRCS+=		miniipc.c
SRCS+=		sdmmc.c
SRCS+=		timer.c

NOMAN=		# defined
NOSANITIZER=	# defined
NORELRO=	# defined
STRIPFLAG=	# nothing

LIBCRT0=	# nothing
LIBCRTI=	# nothing
LIBCRTBEGIN=	# nothing
LIBCRTEND=	# nothing
LIBC=		# nothing

BINDIR=		/usr/mdec
BINMODE=	444

.include <bsd.own.mk>

TEXTADDR=	0x01004000
STACKADDR=	0x01100000
ENTRY=		__start

AFLAGS+=	-mcpu=750

CFLAGS+=	-mcpu=750
CFLAGS+=	${${ACTIVE_CC} == "gcc":? -msdata=none :}
CFLAGS+=	${${ACTIVE_CC} == "gcc":? -mno-strict-align :}
CFLAGS+=	-msoft-float
CFLAGS+=	-ffreestanding
CFLAGS+=	-nostdinc
CFLAGS+=	-Wall -Wmissing-prototypes

CPPFLAGS+=	-D_STANDALONE
CPPFLAGS+=	-DSUPPORT_DOSFS
CPPFLAGS+=	-DLIBSA_FFS_EI
CPPFLAGS+=	-DLIBSA_DISKLABEL_EI

CPPFLAGS+=	-DTEXTADDR=${TEXTADDR}
CPPFLAGS+=	-DSTACKADDR=${STACKADDR}
CPPFLAGS+=	-DPPC_OEA -DDISTANT_KERNEL -D__wii__ -Devbppc=evbppc
CPPFLAGS+=	-DCACHE_LINE_SIZE=32
CPPFLAGS+=	-DEVBPPC_HAS_MBR

CPPFLAGS+=	-I. -I${.CURDIR} -I${.CURDIR}/../../.. -I${.CURDIR}/../../../..

# Follow the suit of Makefile.kern.inc; needed for the lfs64 union
# accessors -- they don't actually dereference the resulting pointer,
# just use it for type-checking.
CWARNFLAGS+=		${CC_WNO_ADDRESS_OF_PACKED_MEMBER}

### find out what to use for libsa
SA_AS= library
SAMISCMAKEFLAGS+=	SA_USE_LOADFILE=yes
SAMISCMAKEFLAGS+=	SA_USE_CREAD=yes
SAMISCMAKEFLAGS+=	SA_INCLUDE_NET=no
.include "${S}/lib/libsa/Makefile.inc"
LIBSA= ${SALIB}

### find out what to use for libkern
KERN_AS= library
.include "${S}/lib/libkern/Makefile.inc"
LIBKERN= ${KERNLIB}

### find out what to use for libz
Z_AS= library
.include "${S}/lib/libz/Makefile.inc"
LIBZ= ${ZLIB}

cleandir distclean: .WAIT cleanlibdir

cleanlibdir:
	-rm -rf lib

LIBLIST= ${LIBSA} ${LIBZ} ${LIBKERN} ${LIBSA}

.include "${S}/conf/newvers_stand.mk"

${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${_MKTARGET_LINK}
	${LD} -N -T ${POWERPC}/conf/kern.ldscript \
	    -Ttext ${TEXTADDR} -e ${ENTRY} -q -X \
	    -o ${PROG} ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}

release: check_RELEASEDIR
	${HOST_INSTALL_FILE} -m ${BINMODE} ${PROG} \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc

.include <bsd.klinks.mk>
.include <bsd.prog.mk>
