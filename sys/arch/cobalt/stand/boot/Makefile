#	$NetBSD: Makefile,v 1.18.12.1 2009/05/13 17:16:37 jym Exp $

NOMAN= # defined

.include <bsd.own.mk>
.include <bsd.sys.mk>		# for HOST_SH

S=	${.CURDIR}/../../../..
MIPS=	${S}/arch/mips
COBALT=	${S}/arch/cobalt
LIBSADIR=	${S}/lib/libsa

# .PATH:	${.CURDIR}/../common

BINMODE?= 444

# XXX SHOULD NOT NEED TO DEFINE THESE!
LIBCRT0=
LIBC=
LIBCRTBEGIN=
LIBCRTEND=

realall: ${PROG}

# Load @15Mb boundary as most (all?) of the Cobalt boxes
# had been shipped with at least 16Mb.
#
# XXX The proper fix is to load at the kernel base address
# and to relocate itself at the end of available memory.
LOAD_ADDRESS?=0x80F00000

COMPORT?=0x0
COMSPEED?=115200
COMPROBE?=0xa020001c

ZSCHAN?=0x01		# 0x01: ZS_CHAN_A, 0x00: ZS_CHAN_B
ZSSPEED?=115200
ZSPROBE?=0xa020001c

AFLAGS+=	-D_LOCORE -D_KERNEL -DASSEMBLER -mno-abicalls

# -I${.CURDIR}/../.. done by Makefile.inc
CPPFLAGS+=	-nostdinc -D_STANDALONE -DNO_ABICALLS -D_NO_PROM_DEFINES
# CPPFLAGS+=	-D_DEBUG
CPPFLAGS+=	-I${.OBJDIR} -I${S} -I${S}/arch -I${LIBSADIR}
CPPFLAGS+=	-DCONS_SERIAL -DCOMPORT=${COMPORT}
CPPFLAGS+=	-DCOMSPEED=${COMSPEED} -DCOMPROBE=${COMPROBE}
CPPFLAGS+=	-DCONS_ZS -DZSCHAN=${ZSCHAN}
CPPFLAGS+=	-DZSSPEED=${ZSSPEED} -DZSPROBE=${ZSPROBE}
CPPFLAGS+=	-DSUPPORT_DHCP -DSUPPORT_BOOTP
#CPPFLAGS+=	-DBOOTP_DEBUG -DNETIF_DEBUG -DETHER_DEBUG -DNFS_DEBUG
#CPPFLAGS+=	-DRPC_DEBUG -DRARP_DEBUG -DNET_DEBUG -DDEBUG -DPARANOID

# compiler flags for smallest code size
CFLAGS=		-Os -mmemcpy -ffreestanding -mno-abicalls -msoft-float -G 128

CFLAGS+=	-Wall -Werror
CFLAGS+=	-Wstrict-prototypes -Wmissing-prototypes -Wpointer-arith
CFLAGS+=	-Wno-pointer-sign

NETBSD_VERS!=	${HOST_SH} ${S}/conf/osrelease.sh
CPPFLAGS+=	-DNETBSD_VERS='"${NETBSD_VERS}"'

LDSCRIPT?=	${MIPS}/conf/stand.ldscript

PROG=		boot
# common sources
SRCS+=		start.S boot.c devopen.c conf.c clock.c bootinfo.c
SRCS+=		prf.c com.c cons.c ns16550.c pciide.c tgets.c wdc.c wd.c
SRCS+=		zs.c
SRCS+=		cache.c pci.c nif_tlp.c tlp.c
SRCS+=		lcd.c

# XXX dev_net.c should really be in libsa, but it doesn't
#     declare ip_convertaddr correctly.
.PATH: ${LIBSADIR}
SRCS+=		dev_net.c

SRCS+=		vers.c
CLEANFILES+=	vers.c

### find out what to use for libkern
KERN_AS=	library
.include "${S}/lib/libkern/Makefile.inc"

### find out what to use for libz
Z_AS=		library
.include "${S}/lib/libz/Makefile.inc"

### find out what to use for libsa
SA_AS=		library
SAMISCMAKEFLAGS+=SA_USE_LOADFILE=yes SA_USE_CREAD=yes
.include "${S}/lib/libsa/Makefile.inc"

LIBS=		${SALIB} ${ZLIB} ${KERNLIB}

.PHONY: vers.c
vers.c: ${.CURDIR}/version
	${HOST_SH} ${S}/conf/newvers_stand.sh ${.CURDIR}/version "cobalt"

${PROG}: ${LDSCRIPT} ${OBJS} ${LIBS}
	${LD} -Map ${PROG}.map -N -x -Ttext ${LOAD_ADDRESS} \
	    -T ${LDSCRIPT} -e start -o ${PROG} ${OBJS} ${LIBS}
	gzip -c9 ${PROG} > ${PROG}.gz
	@${SIZE} ${PROG}

CLEANFILES+=	${PROG}.map ${PROG}.elf ${PROG}.gz

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib

.include <bsd.klinks.mk>
.include <bsd.prog.mk>
