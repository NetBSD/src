#	$NetBSD: Makefile,v 1.21 2001/06/01 23:26:31 jdolecek Exp $

S=	${.CURDIR}/../../../../

BASE= netboot
PROG= ${BASE}.rom
MKMAN=	no
NEWVERSWHAT=	"Network Boot"

SRCS= main.c devopen.c conf.c dev_net.c exec.c

CLEANFILES+= ${ROMSTART} ${BASE}.bin ${BASE}.sym

#CPPFLAGS+= -DDEBUG
#CPPFLAGS+= -DNET_DEBUG
#CPPFLAGS+= -DSUPPORT_BOOTP -DSUPPORT_DHCP
#CPPFLAGS+= -DSUPPORT_TFTP
CPPFLAGS+= -DSUPPORT_RARP -DSUPPORT_BOOTPARAM
CPPFLAGS+= -DSUPPORT_NFS -DNFS_NOSYMLINK
#uncomment if there are problems with memory detection
#CPPFLAGS+= -DCONSERVATIVE_MEMDETECT

#CFLAGS= -O2 -fomit-frame-pointer -fno-defer-pop
CFLAGS+= -Wall

# XXX should go into library
.PATH: ${.CURDIR}/../libsa
SRCS+= nfs.c
CPPFLAGS+= -I${.CURDIR}/../libsa

#ROM_SIZE= 16384
ROM_SIZE= 32768

RELOC= 90000
CPPFLAGS+= -DBOOTROM -DRELOC=0x$(RELOC)

.PATH: ${.CURDIR}/../lib/netif

#USE_NETIF= 3c509
#USE_NETIF= 3c590 #handles 3c900 TPO / combo and 3c905 TX too
# uncomment next and one of the following to make a PCI expansion ROM for 3com
#CPPFLAGS+= -DPCIROM -DPCI_VID=0x10b7 -DPCI_CLASS=0x020000
#CPPFLAGS+= -DPCI_DID=0x5900 # 3c590
#CPPFLAGS+= -DPCI_DID=0x9000 # 3c900 TPO
#CPPFLAGS+= -DPCI_DID=0x9001 # 3c900 combo
#CPPFLAGS+= -DPCI_DID=0x9050 # 3c905 TX

USE_NETIF= pcnet_pci
# uncomment the following to make a PCI expansion ROM for PCNET-PCI
#CPPFLAGS+= -DPCIROM -DPCI_VID=0x1022 -DPCI_DID=0x2000 -DPCI_CLASS=0x020000
#USE_NETIF= pcnet_isapnp #boot device passing to kernel broken

#USE_NETIF= wd80x3 #and uncomment at least one of the two following lines
#CPPFLAGS+= -DSUPPORT_WD80X3
#CPPFLAGS+= -DSUPPORT_SMC_ULTRA

#USE_NETIF= i82557
#CPPFLAGS+= -DPCIROM -DPCI_VID=0x8086 -DPCI_DID=0x1229 -DPCI_CLASS=0x020000

.include "../lib/netif/Makefile.inc"

SAMISCCPPFLAGS+= -DHEAP_START=0x10000 -DHEAP_LIMIT=0x40000
SAMISCCPPFLAGS+= -DNFS_NOSYMLINK -DUDP_NO_CKSUM
SAMISCMAKEFLAGS= SA_USE_CREAD=yes
#I386MISCCPPFLAGS+= -DDISK_DEBUG
#I386MISCCPPFLAGS+= -DSUPPORT_SERIAL -DDIRECT_SERIAL -DCOMCONS_KEYPRESS
I386MISCMAKEFLAGS= I386_INCLUDE_DISK=no I386_INCLUDE_BUS=yes

VERSIONFILE= ${.CURDIR}/version

.include "../Makefile.booters"

conf.o dev_net.o: Makefile
