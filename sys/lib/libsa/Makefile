#	$Id: Makefile,v 1.5 1994/08/22 21:55:58 brezak Exp $

LIB=	sa

#DEBUGFLAGS= -DNETIF_DEBUG -DETHER_DEBUG -DNFS_DEBUG -DRPC_DEBUG -DRARP_DEBUG -DDEBUG -DPARANOID -Wall
#DEBUGFLAGS= -ansi -pedantic -Wall
CFLAGS+=-DSTANDALONE -DCOMPAT_UFS $(DEBUGFLAGS) -I${.CURDIR} -I${.CURDIR}/../..

# stand routines
SRCS+=	alloc.c bcopy.c exit.c exec.c getfile.c gets.c globals.c \
	printf.c strerror.c

# io routines
SRCS+=	close.c dev.c disklabel.c ioctl.c lseek.c open.c read.c \
	stat.c write.c

# network routines
SRCS+=	arp.c ether.c in_cksum.c net.c netif.c rpc.c

# network info services:
SRCS+=	bootp.c rarp.c

# boot filesystems
SRCS+=	ufs.c nfs.c

NOPROFILE=
NOPIC=
OBJMACHINE=

install:

.include <bsd.lib.mk>
