#	$NetBSD: Makefile,v 1.4 1997/05/30 13:43:33 cjs Exp $

LIB=sa

NOPIC=nopic
NOPROFILE=noprofile

# this library is used only for the build
NOINSTALL=1

# Logically src/sys
S=${.CURDIR}/../../../..
DIR_SA=$S/lib/libsa
DIR_KERN=$S/lib/libkern

SRC_net= nfs.c rpc.c net.c ether.c arp.c in_cksum.c \
	 bootparam.c rarp.c

SRC_sa = alloc.c bcopy.c close.c getfile.c memcpy.c open.c \
	 printf.c read.c strerror.c ufs.c 

SRC_kern= ashrdi3.c bcmp.c bzero.c strcmp.c strlen.c

SRC_here= clock.c dev_disk.c devopen.c dvma.c \
	 exec_sun.c gets.c iommu.c mmu.c netif_sun.c\
	 panic.c promboot.c promcons.c promdev.c

SRCS= ${SRC_net} ${SRC_sa} ${SRC_kern} ${SRC_here}

CLEANFILES+=	SRT0.o SRT1.o

# DBG= -DDEBUG -DNETIF_DEBUG -DNFS_DEBUG -DRPC_DEBUG \
#  -DNET_DEBUG -DRARP_DEBUG -DETHER_DEBUG -DDEBUG_PROM

DEFS= -Dsun3x -DSTANDALONE -DCOMPAT_UFS
INCL= -I${.CURDIR} -I${S}/lib/libsa -I${S}
COPTS= -msoft-float
CFLAGS= -O ${COPTS} ${DEFS} ${DBG} ${INCL}

.PATH:  ${DIR_SA} ${DIR_KERN} ${.CURDIR}/../../sun3

all: libsa.a SRT0.o SRT1.o

.include <bsd.lib.mk>
