#	$NetBSD: Makefile,v 1.30 2001/07/30 00:10:20 chs Exp $

LIB=sa

# 
# We're not actually building a profiled version,
# but this way the separate object files get built
#

POBJS=SRT0.o SRT1.o

MKLINT=no
MKPIC=no
MKPROFILE=no

# Logically src/sys
S=${.CURDIR}/../../../..
DIR_SA=${S}/lib/libsa
DIR_KERN=${S}/lib/libkern

.PATH:  ${DIR_SA} ${DIR_KERN}

# DBG= -DDEBUG -DNETIF_DEBUG -DNFS_DEBUG -DRPC_DEBUG \
#  -DNET_DEBUG -DRARP_DEBUG -DETHER_DEBUG -DDEBUG_PROM

DEFS= -Dsun3 -D_STANDALONE
INCL= -I. -I${.CURDIR} -I${S}/lib/libsa -I${S}
CFLAGS= -O2 -msoft-float -fno-defer-pop
CPPFLAGS= ${DEFS} ${DBG} ${INCL}
CLEANFILES= SRT0.o SRT1.o vers.c

SRC_net= net.c ether.c arp.c in_cksum.c rarp.c

SRC_sa = alloc.c bcopy.c close.c getfile.c loadfile.c lseek.c memcpy.c open.c \
	 printf.c read.c sprintf.c strerror.c subr_prf.c twiddle.c

SRC_kern= ashrdi3.c bcmp.c bzero.c inet_addr.c intoa.c \
	memcmp.c memset.c strcmp.c strlen.c strncmp.c

SRC_here= clock.c dev_disk.c devopen.c \
	 gets.c idprom.c netif_sun.c panic.c \
	 promboot.c promcons.c promdev.c \
	 sun3.c sun3x.c vers.c xxboot.c

SRCS= ${SRC_net} ${SRC_sa} ${SRC_kern} ${SRC_here}
SRCS+= ${S}/arch/sun3/sun3/ctrlsp.S

# only needed during build
libinstall::

.include <bsd.lib.mk>

# Nuke these...
LOBJS=
POBJS=
SOBJS=

# Customized rules...

vers.c: ${.CURDIR}/version
	sh ${S}/conf/newvers_stand.sh -MD ${.CURDIR}/version "sun3"
