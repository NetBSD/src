#	$NetBSD: Makefile,v 1.10.2.3 2000/12/08 09:28:43 bouyer Exp $

LIB=sa

# Logically src/sys
DIR_SA=$S/lib/libsa
DIR_KERN=$S/lib/libkern

SRC_net= nfs.c rpc.c net.c ether.c arp.c in_cksum.c netif.c \
	 bootparam.c rarp.c


SRC_sa = alloc.c memcpy.c memset.c close.c exit.c getfile.c gets.c \
	 open.c printf.c read.c strerror.c ufs.c globals.c lseek.c panic.c \
	 closeall.c dev.c dkcksum.c nullfs.c fstat.c twiddle.c sprintf.c \
	 subr_prf.c intoa.c udp.c memcmp.c bcmp.c loadfile.c

SRC_kern= ashrdi3.c strcmp.c strlen.c

SRC_mvme= exec_mvme.c

SRC_here= clock.c chiptotime.c bugdev.c parse_args.c

SRCS= ${SRC_net} ${SRC_sa} ${SRC_kern} ${SRC_mvme} ${SRC_here}

# DBG= -DDEBUG -DNETIF_DEBUG -DNFS_DEBUG -DRPC_DEBUG \
# 	-DNET_DEBUG -DRARP_DEBUG -DETHER_DEBUG

DEFS= ${DBG} #-fno-defer-pop

CLEANFILES+= SRT0.o

.include "../Makefile.booters"

.PATH:  ${DIR_SA} ${DIR_KERN} 

# only needed during build
libinstall::

all realall: lib${LIB}.a SRT0.o

.include <bsd.lib.mk>
