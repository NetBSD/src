#	$NetBSD: Makefile,v 1.1 2002/02/16 03:37:40 thorpej Exp $

S=	${.CURDIR}/../../../../

BASE?= pxeboot
PROG= ${BASE}.pxe
MKMAN=	no
NEWVERSWHAT=	"PXE Boot"

#.PATH.c: ${.CURDIR}/../netboot

SRCS= main.c dev_net.c devopen.c conf.c exec.c pxe.c pxe_call.S

CLEANFILES+= ${PXESTART}

# Various serial line configurations
CPPFLAGS+= -DSUPPORT_SERIAL=CONSDEV_PC -DDIRECT_SERIAL
CPPFLAGS+= -DCONSPEED=19200
#	or
#CPPFLAGS+= -DSUPPORT_SERIAL=CONSDEV_COM0KBD
#	or
#CPPFLAGS+= -DSUPPORT_SERIAL=CONSDEV_AUTO 
#	and maybe 
#CPPFLAGS+= -DDIRECT_SERIAL -DCOMCONS_KEYPRESS -DCONSPEED=57600

CPPFLAGS+= -DSUPPORT_BOOTP -DSUPPORT_DHCP
CPPFLAGS+= -DSUPPORT_TFTP
CPPFLAGS+= -DSUPPORT_NFS

CPPFLAGS+= -DPASS_MEMMAP

#CPPFLAGS+= -DBOOTPASSWD

#CFLAGS= -O2 -fomit-frame-pointer -fno-defer-pop
CFLAGS+= -Wall -Wmissing-prototypes -Wstrict-prototypes -Wno-main

SAMISCCPPFLAGS+= -DHEAP_START=0x10000 -DHEAP_LIMIT=0x30000
#SAMISCMAKEFLAGS+= SA_USE_CREAD=yes	# Read compressed kernels

I386MISCMAKEFLAGS= I386_INCLUDE_DISK=no

VERSIONFILE= ${.CURDIR}/version

.include "../Makefile.booters"
