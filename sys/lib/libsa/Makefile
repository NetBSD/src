#	$Id: Makefile,v 1.3 1994/07/27 07:40:52 cgd Exp $

LIB=	sa

#DEBUGFLAGS= -DNFS_DEBUG -DRPC_DEBUG -DRARP_DEBUG -DDEBUG -DPARANOID -Wall
CFLAGS+=-DSTANDALONE $(DEBUGFLAGS) -I${.CURDIR} -I${.CURDIR}/../.. -DKERNEL

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
