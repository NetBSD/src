#	$NetBSD: Makefile,v 1.4 1997/03/15 22:16:35 perry Exp $

S=	${.CURDIR}/../../../../

PROG= netboot
NOMAN=

SRCS= main.c devopen.c conf.c dev_net.c version.c

CLEANFILES+= ${ROMSTART} ${PROG}.bin ${PROG}.rom ${PROG}.sym ${PROG}.list

#CPPFLAGS+= -DCOMPAT_OLDBOOT
#CPPFLAGS+= -DDEBUG
#should go into SAMISCCPPFLAGS after system upgrade
CPPFLAGS+= -DHEAP_START=0x10000 -DHEAP_LIMIT=0x40000
CPPFLAGS+= -DNFS_NOSYMLINK
#CPPFLAGS+= -DSUPPORT_BOOTP -DSUPPORT_TFTP
CPPFLAGS+= -DSUPPORT_RARP -DSUPPORT_BOOTPARAM -DSUPPORT_NFS

#CFLAGS= -O2 -fomit-frame-pointer -fno-defer-pop
#needs some cleanup in libsa
#CFLAGS+= -Wall

# XXX should go into library
.PATH: ${.CURDIR}/../libsa
#SRCS+= tftp.c
SRCS+= nfswrapper.c
CPPFLAGS+= -I${.CURDIR}/../libsa

${PROG}: ${PROG}.rom
	@# do nothing

install:
	${INSTALL} ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${PROG}.rom ${DESTDIR}${BINDIR}

#ROM_SIZE= 16384
ROM_SIZE= 32768

RELOC= 90000
CPPFLAGS+= -DBOOTROM -DRELOC=0x$(RELOC)

.PATH: ${.CURDIR}/../lib/netif

#USE_NETIF= 3c509
#USE_NETIF= 3c590
#CPPFLAGS+= -DPCIROM -DPCI_VID=0x10b7 -DPCI_DID=0x5900 -DPCI_CLASS=0x020000
USE_NETIF= pcnet_pci
#USE_NETIF= pcnet_isapnp
#USE_NETIF= wd80x3

.include "../lib/netif/Makefile.inc"

#use SAMISCCPPFLAGS after system upgrade
#SAMISCCPPFLAGS+= -DHEAP_START=0x10000 -DHEAP_LIMIT=0x40000
#SAMISCCPPFLAGS+= -DNFS_NOSYMLINK
SAMISCMAKEFLAGS= SA_USE_CREAD=yes
#I386MISCCPPFLAGS+= -DDISK_DEBUG
#I386MISCCPPFLAGS+= -DSUPPORT_SERIAL
I386MISCMAKEFLAGS= I386_INCLUDE_DISK=no I386_INCLUDE_BUS=yes

.include "../Makefile.booters"

conf.o dev_net.o: Makefile
