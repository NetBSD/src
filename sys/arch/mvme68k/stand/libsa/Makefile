#	$NetBSD: Makefile,v 1.10.14.1 2002/02/09 18:09:45 he Exp $

LIB=sa

CLEANFILES+=SRT0.o 

MKPIC=no
MKPROFILE=no
MKLINT=no

# Logically src/sys
S=${.CURDIR}/../../../..
DIR_SA=$S/lib/libsa
DIR_KERN=$S/lib/libkern

SRC_net= nfs.c rpc.c net.c ether.c arp.c in_cksum.c netif.c \
	 bootparam.c rarp.c


SRC_sa = alloc.c bcopy.c memcpy.c close.c exit.c getfile.c gets.c open.c \
	 printf.c read.c strerror.c ufs.c globals.c lseek.c panic.o \
	 closeall.c dev.c dkcksum.c nullfs.c fstat.c twiddle.o sprintf.o \
	 subr_prf.c intoa.c

SRC_kern= ashldi3.c ashrdi3.c bcmp.c bzero.c strcmp.c strlen.c

SRC_mvme= exec_mvme.c

SRC_here= clock.c bugdev.c parse_args.c

SRCS= ${SRC_net} ${SRC_sa} ${SRC_kern} ${SRC_mvme} ${SRC_here}

# DBG= -DDEBUG -DNETIF_DEBUG -DNFS_DEBUG -DRPC_DEBUG \
# 	-DNET_DEBUG -DRARP_DEBUG -DETHER_DEBUG

DEFS= -D_STANDALONE
CPPFLAGS+= -I. -I${.CURDIR}/../libbug -I${S}/lib/libsa -I${S}
COPTS= #-fno-defer-pop
CFLAGS= -O2 ${COPTS} ${DEFS} ${DBG}

.PATH:  ${DIR_SA} ${DIR_KERN} 

all: libsa.a SRT0.o 

# only needed during build
libinstall::

.include <bsd.lib.mk>
