#	$NetBSD: Makefile,v 1.16 2001/06/10 14:12:48 scw Exp $

LIB=sa

# Logically src/sys
DIR_SA=$S/lib/libsa
DIR_KERN=$S/lib/libkern

SRC_net= nfs.c rpc.c net.c ether.c arp.c in_cksum.c netif.c \
	 bootparam.c rarp.c dev_net.c bootp.c


SRC_sa = alloc.c memcpy.c memset.c close.c exit.c getfile.c gets.c \
	 open.c printf.c read.c strerror.c ufs.c globals.c lseek.c panic.c \
	 closeall.c dev.c dkcksum.c nullfs.c fstat.c twiddle.c sprintf.c \
	 subr_prf.c intoa.c udp.c memcmp.c bcmp.c loadfile.c dev_net.c

SRC_kern= ashrdi3.c strcmp.c strlen.c strncpy.c inet_addr.c

SRC_mvme= exec_mvme.c

SRC_here= clock.c chiptotime.c bugdev.c parse_args.c

SRCS= ${SRC_net} ${SRC_sa} ${SRC_kern} ${SRC_mvme} ${SRC_here}

# DBG= -DDEBUG -DNETIF_DEBUG -DNFS_DEBUG -DRPC_DEBUG \
# 	-DNET_DEBUG -DRARP_DEBUG -DETHER_DEBUG

DEFS= ${DBG} #-fno-defer-pop

CPPFLAGS+= -DSUPPORT_BOOTP -DSUPPORT_DHCP
CPPFLAGS+= -DSUPPORT_RARP -DSUPPORT_BOOTPARAM
CPPFLAGS+= -DSUPPORT_NFS -DNFS_NOSYMLINK

CLEANFILES+= SRT0.o

.include "../Makefile.booters"

.PATH:  ${DIR_SA} ${DIR_KERN} 

# only needed during build
libinstall::

all realall: lib${LIB}.a SRT0.o

.include <bsd.lib.mk>
