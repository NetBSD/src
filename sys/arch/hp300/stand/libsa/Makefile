#	$NetBSD: Makefile,v 1.1 1995/08/04 07:55:49 thorpej Exp $

LIB=	sa

.PATH:	${.CURDIR}/../../../../lib/libsa

DEBUGFLAGS=-DNETIF_DEBUG

CFLAGS+=-DSTANDALONE -DCOMPAT_UFS ${DEBUGFLAGS}
CFLAGS+=-I${.CURDIR}/../../../..  -I${.CURDIR}/../../../../lib/libsa

# stand routines
SRCS=	alloc.c bcopy.c exit.c exec.c getfile.c gets.c globals.c \
	printf.c strerror.c

# io routines
SRCS+=	close.c dev.c disklabel.c ioctl.c lseek.c open.c read.c \
	stat.c write.c

# network routines
SRCS+=	arp.c ether.c in_cksum.c net.c netif.c rpc.c

# network info services
SRCS+=	bootp.c rarp.c

# boot filesystems
SRCS+=	ufs.c nfs.c

NOPROFILE=
NOPIC=
OBJMACHINE=

install:

.include <bsd.lib.mk>
