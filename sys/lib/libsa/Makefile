#	$NetBSD: Makefile,v 1.11 1996/01/13 22:25:32 leo Exp $

LIB=	sa

DIR=${SAREL}${SADIR}

.PATH: ${DIR}

#DEBUGFLAGS= -DNETIF_DEBUG -DETHER_DEBUG -DNFS_DEBUG -DRPC_DEBUG -DRARP_DEBUG -DDEBUG -DPARANOID -Wall
#DEBUGFLAGS= -ansi -pedantic -Wall
CFLAGS+=-DSTANDALONE -DCOMPAT_UFS $(DEBUGFLAGS) -I${DIR} -I${DIR}/../..

# stand routines
SRCS+=	alloc.c bcopy.c exit.c exec.c getfile.c gets.c globals.c \
	memcpy.c printf.c strerror.c

# io routines
SRCS+=	close.c closeall.c dev.c disklabel.c dkcksum.c ioctl.c \
	lseek.c open.c nullfs.c read.c stat.c fstat.c write.c

.if !defined(NO_NET)
# network routines
SRCS+=	arp.c ether.c in_cksum.c net.c netif.c rpc.c
.endif

# network info services:
SRCS+=	bootp.c rarp.c bootparam.c

# boot filesystems
SRCS+=	ufs.c nfs.c

NOPROFILE=
NOPIC=
OBJMACHINE=

install:

.include <bsd.lib.mk>
