#	$NetBSD: Makefile,v 1.7 1997/10/13 21:59:24 gwr Exp $

LIB=sa

NOLINT=
NOPIC=
NOPROFILE=

# Logically src/sys
S=${.CURDIR}/../../../..
DIR_SA=$S/lib/libsa
DIR_KERN=$S/lib/libkern

.PATH:  ${DIR_SA} ${DIR_KERN}

# DBG= -DDEBUG -DNETIF_DEBUG -DNFS_DEBUG -DRPC_DEBUG \
#  -DNET_DEBUG -DRARP_DEBUG -DETHER_DEBUG -DDEBUG_PROM
DEFS= -Dsun3x -D_STANDALONE
INCL= -I. -I${.CURDIR} -I${S}/lib/libsa -I${S}
CFLAGS= -O2 -msoft-float -fno-defer-pop
CPPFLAGS= ${DEFS} ${DBG} ${INCL}
CLEANFILES= SRT0.o SRT1.o machine m68k

SRC_net= net.c ether.c arp.c in_cksum.c rarp.c

SRC_sa = alloc.c bcopy.c close.c getfile.c memcpy.c open.c \
	 printf.c read.c strerror.c

SRC_kern= ashrdi3.c bcmp.c bzero.c strcmp.c strlen.c

SRC_here= clock.c dev_disk.c devopen.c dvma.c exec_sun.c \
	 gets.c idprom.c iommu.c mmu.c netif_sun.c \
	 panic.c promboot.c promcons.c promdev.c

SRCS= ${SRC_net} ${SRC_sa} ${SRC_kern} ${SRC_here}

all: libsa.a SRT0.o SRT1.o

# only needed during build
libinstall::

.include <bsd.lib.mk>

# Nuke these...
LOBJS=
POBJS=
SOBJS=

# Customized rules...

${OBJS} SRT0.o SRT1.o : machine m68k

machine :
	-rm -f $@
	ln -s ${S}/arch/sun3x/include $@

m68k :
	-rm -f $@
	ln -s ${S}/arch/m68k/include $@

# This is for programs using ../Makefile.inc
echo_objdir:
	@echo ${.OBJDIR}

