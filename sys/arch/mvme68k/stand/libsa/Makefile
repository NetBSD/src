#	$NetBSD: Makefile,v 1.7 1999/02/13 02:54:47 lukem Exp $

LIB=sa

CLEANFILES+=SRT0.o 

MKPIC=no
MKPROFILE=no

# Logically src/sys
S=${.CURDIR}/../../../..
DIR_SA=$S/lib/libsa
DIR_KERN=$S/lib/libkern

SRC_net= nfs.c rpc.c net.c ether.c arp.c in_cksum.c netif.c \
	 bootparam.c rarp.c


SRC_sa = alloc.c bcopy.c memcpy.c close.c exit.c getfile.c gets.c open.c \
	 printf.c read.c strerror.c ufs.c globals.c lseek.c \
	 closeall.c dev.c dkcksum.c nullfs.c fstat.c

SRC_kern= ashrdi3.c bcmp.c bzero.c strcmp.c strlen.c

SRC_mvme= exec_mvme.c

SRC_here= clock.c bugdev.c parse_args.c

SRCS= ${SRC_net} ${SRC_sa} ${SRC_kern} ${SRC_mvme} ${SRC_here}

# DBG= -DDEBUG -DNETIF_DEBUG -DNFS_DEBUG -DRPC_DEBUG \
# 	-DNET_DEBUG -DRARP_DEBUG -DETHER_DEBUG

#DEFS= -DCOMPAT_UFS
CPPFLAGS+= -I. -I${.CURDIR}/../libbug -I${S}/lib/libsa -I${S}
COPTS= #-fno-defer-pop
CFLAGS= -O2 ${COPTS} ${DEFS} ${DBG}

.PATH:  ${DIR_SA} ${DIR_KERN} 

all: libsa.a SRT0.o 

# only needed during build
libinstall::

.include <bsd.lib.mk>
