#	$NetBSD: Makefile,v 1.14 1999/01/29 18:46:15 christos Exp $

S=	${.CURDIR}/../../../../

BASE= biosboot
PROG= ${BASE}.sym
NOMAN=
NEWVERSWHAT=	"BIOS Boot"

SRCS= main.c devopen.c conf.c exec.c

CLEANFILES+= ${BSSTART}

CPPFLAGS+= -DCOMPAT_OLDBOOT -DCOMPAT_386BSD_MBRPART

#Sample use of serial line debugger
#CPPFLAGS+= -DSUPPORT_SERIAL=CONSDEV_COM0KBD
#	or
#CPPFLAGS+= -DSUPPORT_SERIAL=CONSDEV_AUTO
#	and maybe
#CPPFLAGS+= -DDIRECT_SERIAL -DCOMCONS_KEYPRESS -DCONSPEED=57600

#CPPFLAGS+= -DPASS_BIOSGEOM
#uncomment if there are problems with memory detection
#CPPFLAGS+= -DCONSERVATIVE_MEMDETECT
#increase MAXFLENTRIES if "installboot" complains about
# "not enough fragment space in bootcode" (default: 10)
#CPPFLAGS+= -DMAXFLENTRIES=15
#set PRIM_LOADSZ to <=9 to make a 720k boot floppy (default: 15)
#CPPFLAGS+= -DPRIM_LOADSZ=9

#CFLAGS= -O2 -fomit-frame-pointer -fno-defer-pop
CFLAGS+= -Wall

# XXX should go into library
SRCS+= ls.c
.PATH: ${.CURDIR}/../libsa

SAMISCCPPFLAGS+= -DHEAP_START=0x10000 -DHEAP_LIMIT=0x40000
SAMISCMAKEFLAGS= SA_USE_CREAD=yes SA_INCLUDE_NET=no

.include "../Makefile.booters"
