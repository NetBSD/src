#	$NetBSD: Makefile,v 1.2.6.2 2002/06/23 17:38:38 jdolecek Exp $

LIB=sa

# Logically src/sys
DIR_SA=$S/lib/libsa
DIR_KERN=$S/lib/libkern

SRC_net= nfs.c rpc.c net.c ether.c arp.c in_cksum.c netif.c \
	 bootparam.c rarp.c dev_net.c bootp.c


SRC_sa = alloc.c memcpy.c memset.c close.c exit.c files.c getfile.c gets.c \
	 open.c printf.c read.c strerror.c ufs.c globals.c lseek.c panic.c \
	 closeall.c dev.c dkcksum.c nullfs.c fstat.c twiddle.c sprintf.c \
	 subr_prf.c intoa.c udp.c memcmp.c loadfile.c loadfile_aout.c \
	 loadfile_elf32.c dev_net.c

# XXX Needed until libsa's memcmp.c doesn't depend on bcmp...
SRC_sa+= bcmp.c

SRC_kern= ashldi3.c ashrdi3.c strcmp.c strlen.c strncpy.c inet_addr.c

SRC_here= bugsyscalls.S exec_mvme.c parse_args.c getchar.c putchar.c
SRC_here+= if_bug.c clock.c

SRCS= ${SRC_net} ${SRC_sa} ${SRC_kern} ${SRC_here}

# DBG= -DDEBUG -DNETIF_DEBUG -DNFS_DEBUG -DRPC_DEBUG \
# 	-DNET_DEBUG -DRARP_DEBUG -DETHER_DEBUG

DEFS= ${DBG} #-fno-defer-pop

CPPFLAGS+= -DSUPPORT_BOOTP -DSUPPORT_DHCP
#CPPFLAGS+= -DSUPPORT_DHCP
#CPPFLAGS+= -DSUPPORT_RARP -DSUPPORT_BOOTPARAM
CPPFLAGS+= -DSUPPORT_NFS -DNFS_NOSYMLINK

CLEANFILES+= srt0.o

.include "../Makefile.booters"

.PATH:  ${DIR_SA} ${DIR_KERN} 

# only needed during build
libinstall::

all realall: lib${LIB}.a srt0.o

.include <bsd.own.mk>
.undef DESTDIR
.include <bsd.lib.mk>
