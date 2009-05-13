#	$NetBSD: Makefile,v 1.5.18.1 2009/05/13 17:18:21 jym Exp $

PROG=	ip30boot

SRCS=	start64.S boot.c bootinfo.c conf.c devopen.c putchar.c getchar.c \
	getopt.c disk.c

CLEANFILES+=	boot.map

.include <bsd.sys.mk>		# for HOST_SH

# $S must correspond to the top of the 'sys' tree
S=	${.CURDIR}/../../../..

BINMODE?=	444

realall: ${PROG}

.PATH:		${.CURDIR}/../common
AFLAGS+=	-D_LOCORE -D_KERNEL -mno-abicalls -mabi=64 -D_LP64
CPPFLAGS+=	-nostdinc -D_STANDALONE -DNO_ABICALLS -I${.OBJDIR} -I${S}
# compiler flags for smallest code size
CFLAGS=		-ffreestanding -Os -Wall -Werror -mno-abicalls -msoft-float -G 1024 -mabi=64 -D_LP64
LDBUG=		-T $S/arch/mips/conf/stand.ldscript
NETBSD_VERS!=	${HOST_SH} ${.CURDIR}/../../../../conf/osrelease.sh
CPPFLAGS+=	-DNETBSD_VERS='"${NETBSD_VERS}"'

CPPFLAGS+=	-Dsgimips

# PROG set by parent.
NOMAN=		# defined

# We load the kernel at 420K in from the start of RAM to give the boot
# loader plenty of breathing room.  Load the boot loader starting at
# the second page of RAM.

LOAD_ADDRESS_IP30?=	0xa800000020080000

# if there is a 'version' file, add rule for vers.c and add it to SRCS
# and CLEANFILES
.if exists(version)
.PHONY: vers.c
vers.c: ${.CURDIR}/version
	${HOST_SH} ${S}/conf/newvers_stand.sh ${.CURDIR}/version "sgimips"

SRCS+=	vers.c
CLEANFILES+= vers.c
.endif

### find out what to use for libkern
KERN_AS=	library
KERNMISCMAKEFLAGS+=	LD="${LD} -m elf64btsmip" RANLIB="true"
.include "${S}/lib/libkern/Makefile.inc"
LIBKERN=	${KERNLIB}

### find out what to use for libz
Z_AS=		library
ZMISCMAKEFLAGS+=	LD="${LD} -m elf64btsmip" RANLIB="true"
.include "${S}/lib/libz/Makefile.inc"
LIBZ=		${ZLIB}

### find out what to use for libsa
SA_AS=		library
SAMISCMAKEFLAGS+=	SA_USE_LOADFILE=yes SA_USE_CREAD=yes \
			LD="${LD} -m elf64btsmip" RANLIB="true"
.include "${S}/lib/libsa/Makefile.inc"
LIBSA=		${SALIB}


LIBS=		${LIBSA} ${LIBZ} ${LIBSA} ${LIBKERN}

ip30boot: ${OBJS} ${LIBS}
	${LD} -m elf64btsmip -Map boot.map -x -Ttext ${LOAD_ADDRESS_IP30} \
		${LDBUG} -e start -o ${.TARGET} ${OBJS} ${LIBS}

cleandir distclean: cleanlibdir
cleanlibdir:
	-rm -rf lib

.include <bsd.klinks.mk>
.include <bsd.prog.mk>
