#	$NetBSD: Makefile,v 1.1.1.1 1995/06/09 22:26:18 gwr Exp $

LIB=net

NOPIC=nopic
NOPROFILE=noprofile

# Logically src/sys
S=../../../..
DIR_SA=$S/lib/libsa

# Netboot stuff - temporary local copies of:
SRC_mynet= bootparam.c nfs.c rpc.c
SRC_sanet= arp.c ether.c in_cksum.c net.c rarp.c

SRCS= ${SRC_mynet} ${SRC_sanet}

# DBG= -DDEBUG -DNETIF_DEBUG -DNFS_DEBUG -DRPC_DEBUG \
# 	-DNET_DEBUG -DRARP_DEBUG -DETHER_DEBUG

DEFS= -DCOMPAT_UFS -DSUN_BOOTPARAMS
INCL= -I. -I../libsa -I${S}/lib/libsa -I${S}
COPTS= -msoft-float -fno-defer-pop -fno-omit-frame-pointer
CFLAGS= -O ${COPTS} ${DEFS} ${DBG} ${INCL}

.PATH:  ${DIR_SA}

all: libnet.a

install:

.include <bsd.lib.mk>
