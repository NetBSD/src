#	$NetBSD: Makefile,v 1.22 1999/12/17 07:28:01 jeremy Exp $

LIB=sa

MKLINT=no
MKPIC=no
MKPROFILE=no

# Logically src/sys
S=${.CURDIR}/../../../..
DIR_SA=$S/lib/libsa
DIR_KERN=$S/lib/libkern

.PATH:  ${DIR_SA} ${DIR_KERN}

# DBG= -DDEBUG -DNETIF_DEBUG -DNFS_DEBUG -DRPC_DEBUG \
#  -DNET_DEBUG -DRARP_DEBUG -DETHER_DEBUG -DDEBUG_PROM

DEFS= -Dsun3 -D_STANDALONE
INCL= -I. -I${.CURDIR} -I${S}/lib/libsa -I${S}
CFLAGS= -O2 -msoft-float -fno-defer-pop
CPPFLAGS= ${DEFS} ${DBG} ${INCL}
CLEANFILES= SRT0.o SRT1.o machine m68k

SRC_net= net.c ether.c arp.c in_cksum.c rarp.c

SRC_sa = alloc.c bcopy.c close.c getfile.c memcpy.c open.c \
	 printf.c read.c sprintf.c strerror.c subr_prf.c twiddle.c

SRC_kern= ashrdi3.c bcmp.c bzero.c inet_addr.c intoa.c \
	strcmp.c strlen.c strncmp.c

SRC_here= clock.c dev_disk.c devopen.c exec_sun.c \
	 gets.c idprom.c netif_sun.c panic.c \
	 promboot.c promcons.c promdev.c \
	 sun3.c sun3x.c version.c

SRCS= ${SRC_net} ${SRC_sa} ${SRC_kern} ${SRC_here}
OBJS= ctrlsp.o

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
	ln -s ${S}/arch/sun3/include $@

m68k :
	-rm -f $@
	ln -s ${S}/arch/m68k/include $@

ctrlsp.o : ${S}/arch/sun3/sun3/ctrlsp.S
	${COMPILE.S} ${.IMPSRC}

# This is for programs using ../Makefile.inc
echo_objdir:
	@echo ${.OBJDIR}

