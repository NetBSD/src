#	$NetBSD: Makefile,v 1.5 1995/06/16 07:23:02 jtc Exp $

LIB=sa

NOPIC=nopic
NOPROFILE=noprofile

# Logically src/sys
S=${.CURDIR}/../../../..
DIR_SA=$S/lib/libsa
DIR_KERN=$S/lib/libkern

SRC_sa = alloc.c bcopy.c close.c getfile.c open.c \
	 printf.c read.c strerror.c ufs.c 

SRC_kern= ashrdi3.c bcmp.c bzero.c strcmp.c strlen.c

SRC_sun3= control.c ctrlsp.S

SRC_here= clock.c dev_disk.c devopen.c dvma.c \
	 exec_sun.c gets.c netif_sun.c panic.c \
	 promboot.c promcons.c promdev.c

SRCS= ${SRC_sa} ${SRC_kern} ${SRC_sun3} ${SRC_here}

# DBG= -DDEBUG -DNETIF_DEBUG -DNFS_DEBUG -DRPC_DEBUG \
# 	-DNET_DEBUG -DRARP_DEBUG -DETHER_DEBUG

# XXX - Temporary include hack...
SRC_net= ../libnet

DEFS= -DCOMPAT_UFS -DSUN_BOOTPARAMS
INCL= -I. -I${SRC_net} -I${S}/lib/libsa -I${S}
COPTS= -msoft-float -fno-defer-pop -fno-omit-frame-pointer
CFLAGS= -O ${COPTS} ${DEFS} ${DBG} ${INCL}

.PATH:  ${DIR_SA} ${DIR_KERN} ../../sun3

all: libsa.a SRT0.o SRT1.o

install:

.include <bsd.lib.mk>
