#	$NetBSD: Makefile,v 1.43 2003/04/01 21:25:34 mycroft Exp $

S=	${.CURDIR}/../../../../

BASE?= biosboot
PROG= ${BASE}.sym
NOMAN=	# defined
NEWVERSWHAT=	"BIOS Boot"
STARTFILE=	${BSSTART}
RELOC=		0x0

.if (${BASE} != "biosboot")
.PATH.c: ${.CURDIR}/../biosboot/
.endif

SRCS= main.c devopen.c conf.c exec.c

CPPFLAGS+= -DSLOW	# for libz
CPPFLAGS+= -DCOMPAT_386BSD_MBRPART -DSUPPORT_PS2

.if (${BASE} == "biosboot")
# Various serial line configurations
CPPFLAGS+= -DSUPPORT_SERIAL=CONSDEV_PC -DDIRECT_SERIAL
#	or
#CPPFLAGS+= -DSUPPORT_SERIAL=CONSDEV_COM0KBD
#	or
#CPPFLAGS+= -DSUPPORT_SERIAL=CONSDEV_AUTO
#	and maybe
#CPPFLAGS+= -DDIRECT_SERIAL -DCOMCONS_KEYPRESS -DCONSPEED=57600
.endif

.if (${BASE} == "biosboot_com0")
CPPFLAGS+= -DSUPPORT_SERIAL=CONSDEV_COM0 -DDIRECT_SERIAL
.endif

.if (${BASE} == "biosboot_resetvideo")
CPPFLAGS+= -DRESET_VIDEO
CPPFLAGS+= -DSUPPORT_SERIAL=CONSDEV_PC -DDIRECT_SERIAL
.endif

.if defined(BIOSBOOT_CONSPEED)
BASE:=	${BASE}_${BIOSBOOT_CONSPEED}
CPPFLAGS+= -DCONSPEED=${BIOSBOOT_CONSPEED}
.endif

CPPFLAGS+= -DSUPPORT_USTARFS
#CPPFLAGS+= -DSUPPORT_DOSFS
CPPFLAGS+= -DPASS_BIOSGEOM
CPPFLAGS+= -DPASS_MEMMAP
#CPPFLAGS+= -DBOOTPASSWD

#uncomment if there are problems with memory detection
#CPPFLAGS+= -DCONSERVATIVE_MEMDETECT
#increase MAXFLENTRIES if "installboot" complains about
# "not enough fragment space in bootcode" (default: 20)
#CPPFLAGS+= -DMAXFLENTRIES=30
#set PRIM_LOADSZ to <=9 to make a 720k boot floppy (default: 15)
#CPPFLAGS+= -DPRIM_LOADSZ=9

#CFLAGS= -O2 -fomit-frame-pointer -fno-defer-pop
CFLAGS+= -Wall -Wmissing-prototypes -Wstrict-prototypes -Wno-main

# The biosboot code is linked to 'virtual' address of zero and is
# loaded at physical address 0x10000.
# With these values there is 128kb for code, data and stack.
# XXX The values should be determined from _end.
CPPFLAGS+= -DSTACK_START=0x10000
SAMISCCPPFLAGS+= -DHEAP_START=0x20000 -DHEAP_LIMIT=0x50000
SAMISCMAKEFLAGS+= SA_USE_CREAD=yes	# Read compressed kernels
SAMISCMAKEFLAGS+= SA_INCLUDE_NET=no	# Netboot via TFTP, NFS

.if (${BASE} == "biosboot")
VERSIONFILE= ${.CURDIR}/version
.else
VERSIONFILE= ${.CURDIR}/../biosboot/version
.endif

.include "../Makefile.booters"
