#	$NetBSD: Makefile,v 1.1 2012/01/30 03:28:34 nisimura Exp $

S=		${.CURDIR}/../../../..
PROG=		bootmini2440
SRCS=		entry.S main.c devopen.c netif.c dev_net.c dm9k.c dev_sdmmc.c
SRCS+=		s3csdi.c vers.c

.include <bsd.own.mk>

CLEANFILES+=	vers.c ${PROG}.elf
CFLAGS+=	-Wall -Wno-main -ffreestanding -march=armv4
CPPFLAGS+=	-D_STANDALONE -DSUPPORT_DHCP 
CPPFLAGS+=	-DDM9000MAC="0x08,0x08,0x11,0x18,0x12,0x27" 
CPPFLAGS+=	-DDEFAULT_BOOTFILE="ld0e:netbsd;net:"
CPPFLAGS+=	-nostdinc -I. -I${.CURDIR} -I${.OBJDIR} -I${S} -I${S}/arch
DBG=		

LIBCRT0=	# nothing
LIBCRTBEGIN=	# nothing
LIBCRTEND=	# nothing
LIBC=		# nothing

MAN=		# no manual page
NOMAN=		# defined
STRIPFLAG=
BINMODE=	444

RELOC=		0x30A00000
ENTRY=		_start

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
SACPPFLAGS=	-DLIBSA_PRINTF_WIDTH_SUPPORT
.include "${S}/lib/libsa/Makefile.inc"
LIBSA=		${SALIB}

.PHONY: vers.c
vers.c: version
	${HOST_SH} ${S}/conf/newvers_stand.sh -K \
	    ${${MKREPRO} == "yes" :?:-D} ${.CURDIR}/version "evbarm"

${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${LD} -N -Ttext ${RELOC} -Bstatic -e ${ENTRY} -o ${.TARGET}.elf \
	    ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${OBJCOPY} -S -O binary ${.TARGET}.elf ${.TARGET}


.include <bsd.klinks.mk>
.include <bsd.prog.mk>

cleandir distclean: .WAIT cleanlibdir

cleanlibdir:
	-rm -rf lib

