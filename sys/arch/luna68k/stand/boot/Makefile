#	$NetBSD: Makefile,v 1.13.6.1 2017/11/22 15:20:15 martin Exp $
#	@(#)Makefile	8.2 (Berkeley) 8/15/93

NOMAN= # defined
NOPIE= # defined

.include <bsd.own.mk>

S= ${.CURDIR}/../../../..
LIBSADIR=	${S}/lib/libsa

CPPFLAGS+=	-nostdinc -D_STANDALONE
CPPFLAGS+=	-I${.CURDIR} -I${.OBJDIR} -I${S} -I${S}/arch

CPPFLAGS+=	-DSUPPORT_DISK
#CPPFLAGS+=	-DSUPPORT_TAPE
CPPFLAGS+=	-DSUPPORT_ETHERNET
CPPFLAGS+=	-DSUPPORT_DHCP -DSUPPORT_BOOTP
#CPPFLAGS+=	-DBOOTP_DEBUG -DNETIF_DEBUG -DETHER_DEBUG -DNFS_DEBUG
#CPPFLAGS+=	-DRPC_DEBUG -DRARP_DEBUG -DNET_DEBUG -DDEBUG -DPARANOID
CPPFLAGS+=	-DLIBSA_ENABLE_LS_OP
CPPFLAGS+=	-DLIBSA_PRINTF_WIDTH_SUPPORT
CPPFLAGS+=	-DLIBSA_CREAD_NOCRC

CFLAGS=		-Os -msoft-float
CFLAGS+=	-ffreestanding
CFLAGS+=	-Wall -Werror
CFLAGS+=	-Wstrict-prototypes -Wmissing-prototypes -Wpointer-arith
CFLAGS+=	-Wno-pointer-sign

TEXTADDR=	700000
LDSCRIPT=	${.CURDIR}/boot.ldscript
LINKFORMAT=	-static -N -Ttext ${TEXTADDR} -T ${LDSCRIPT}

SRCS=	locore.S
SRCS+=	init_main.c autoconf.c
SRCS+=	trap.c
SRCS+=	devopen.c
SRCS+=	conf.c
SRCS+=	machdep.c
SRCS+=	getline.c parse.c
SRCS+=	boot.c
SRCS+=	cons.c prf.c awaitkey.c
SRCS+=	romcons.c
SRCS+=	sio.c
SRCS+=	bmc.c bmd.c screen.c font.c kbd.c
SRCS+=	scsi.c sc.c sd.c
#SRCS+=	st.c tape.c
SRCS+=	disklabel.c
#SRCS+=	fsdump.c
SRCS+=	ufs_disksubr.c
SRCS+=	ls.c

# netboot support
SRCS+=	if_le.c lance.c getsecs.c
.PATH: ${LIBSADIR}
SRCS+=	dev_net.c

PROG=   boot

NEWVERSWHAT=	"${PROG}"

### find out what to use for libkern
KERN_AS=	library
.include "${S}/lib/libkern/Makefile.inc"

### find out what to use for libz
Z_AS=		library
.include "${S}/lib/libz/Makefile.inc"

### find out what to use for libsa
SA_AS=		library
SAMISCMAKEFLAGS+=SA_USE_LOADFILE=yes SA_USE_CREAD=yes SA_ENABLE_LS_OP=yes
.include "${S}/lib/libsa/Makefile.inc"

LIBS=	${SALIB} ${ZLIB} ${KERNLIB}

.include "${S}/conf/newvers_stand.mk"

${PROG}: ${LDSCRIPT} ${OBJS} ${LIBS}
	${LD} ${LINKFORMAT} -x -o ${PROG}.elf ${OBJS} ${LIBS}
	${ELF2AOUT} ${PROG}.elf ${PROG}.aout
	mv ${PROG}.aout ${PROG}

CLEANFILES+=	${PROG}.aout ${PROG}.elf

cleandir distclean: .WAIT cleanlibdir

cleanlibdir:
	-rm -rf lib

.include <bsd.klinks.mk>
.include <bsd.prog.mk>
