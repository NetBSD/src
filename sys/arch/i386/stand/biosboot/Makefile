#	$NetBSD: Makefile,v 1.10 1997/09/28 11:47:30 drochner Exp $

S=	${.CURDIR}/../../../../

BASE= biosboot
PROG= ${BASE}.sym
NOMAN=
NEWVERSWHAT=	"BIOS Boot"

SRCS= main.c devopen.c conf.c

CLEANFILES+= ${BSSTART}

CPPFLAGS+= -DCOMPAT_OLDBOOT
#CPPFLAGS+= -DSUPPORT_SERIAL
#CPPFLAGS+= -DDIRECT_SERIAL -DCOMCONS_KEYPRESS -DCONSPEED=38400
#CPPFLAGS+= -DPASS_BIOSGEOM
#uncomment if there are problems with memory detection
#CPPFLAGS+= -DCONSERVATIVE_MEMDETECT
#increase MAXFLENTRIES if "installboot" complains about
# "not enough fragment space in bootcode" (default: 5)
#CPPFLAGS+= -DMAXFLENTRIES=10
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
