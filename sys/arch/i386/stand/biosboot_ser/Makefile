#	$NetBSD: Makefile,v 1.9.2.2 2002/02/17 20:03:09 thorpej Exp $

S=	${.CURDIR}/../../../../

BASE= biosboot_ser
PROG= ${BASE}.sym
NOMAN=	# defined
NEWVERSWHAT=	"BIOS Boot"
STARTFILE=	${BSSTART}
RELOC=		0x0

.PATH: ${.CURDIR}/../biosboot/
SRCS= main.c devopen.c conf.c exec.c

CPPFLAGS+= -DCOMPAT_386BSD_MBRPART -DDEBUG

#Sample use of serial line debugger
#CPPFLAGS+= -DSUPPORT_SERIAL=CONSDEV_COM0KBD
#	or
CPPFLAGS+= -DSUPPORT_SERIAL=CONSDEV_AUTO -DDIRECT_SERIAL
#	and maybe
#CPPFLAGS+= -DDIRECT_SERIAL -DCOMCONS_KEYPRESS -DCONSPEED=57600

CPPFLAGS+= -DPASS_BIOSGEOM
CPPFLAGS+= -DPASS_MEMMAP
#uncomment if there are problems with memory detection
#CPPFLAGS+= -DCONSERVATIVE_MEMDETECT
#increase MAXFLENTRIES if "installboot" complains about
# "not enough fragment space in bootcode" (default: 10)
#CPPFLAGS+= -DMAXFLENTRIES=15
#set PRIM_LOADSZ to <=9 to make a 720k boot floppy (default: 15)
#CPPFLAGS+= -DPRIM_LOADSZ=9

#CFLAGS= -O2 -fomit-frame-pointer -fno-defer-pop
CFLAGS+= -Wall -Wmissing-prototypes -Wstrict-prototypes -Wno-main

SAMISCCPPFLAGS+= -DHEAP_START=0x10000 -DHEAP_LIMIT=0x40000
SAMISCMAKEFLAGS= SA_USE_CREAD=yes	# Read compressed kernels
SAMISCMAKEFLAGS= SA_INCLUDE_NET=no	# Support netboot 

VERSIONFILE= ${.CURDIR}/../biosboot/version

.include "../Makefile.booters"
