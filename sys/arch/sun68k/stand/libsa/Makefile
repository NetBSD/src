#	$NetBSD: Makefile,v 1.8 2002/03/22 00:30:06 fredette Exp $

LIB=sa

# 
# We're not actually building a profiled version,
# but this way the separate object files get built
#

POBJS=SRT0.o SRT1.o

NOLINT=# defined
NOPIC=# defined
NOPROFILE=# defined

# Logically src/sys
S=${.CURDIR}/../../../..
DIR_SA=${S}/lib/libsa
DIR_KERN=${S}/lib/libkern

.PATH:  ${DIR_SA} ${DIR_KERN}

# DBG= -DDEBUG -DNETIF_DEBUG -DNFS_DEBUG -DRPC_DEBUG \
#  -DNET_DEBUG -DRARP_DEBUG -DETHER_DEBUG -DDEBUG_PROM

DEFS= -Dsun3 -D_STANDALONE
INCL= -I. -I${.CURDIR} -I${S}/lib/libsa -I${S}
AFLAGS= -Wa,-mc68020 -Wa,-mc68851
CFLAGS= -mc68000 -Wa,-mc68010 -Wa,-m68851 -Os -msoft-float -fno-defer-pop
CPPFLAGS= ${DEFS} ${DBG} ${INCL}
CLEANFILES= SRT0.o SRT1.o vers.c

SRC_net= net.c ether.c arp.c in_cksum.c rarp.c

SRC_sa = alloc.c bcopy.c close.c files.c getfile.c loadfile.c loadfile_aout.c \
	loadfile_elf32.c lseek.c memcpy.c open.c printf.c read.c sprintf.c \
	strerror.c subr_prf.c twiddle.c

SRC_kern= ashldi3.c ashrdi3.c bcmp.c bzero.c inet_addr.c intoa.c \
	memcmp.c memset.c strcmp.c strlen.c strncmp.c

SRC_here= clock.c dev_disk.c devopen.c \
	 gets.c idprom.c netif_sun.c panic.c \
	 promboot.c promcons.c promdev.c \
	 sun2.c sun3.c sun3x.c vers.c xxboot.c

SRCS= ${SRC_net} ${SRC_sa} ${SRC_kern} ${SRC_here}
SRCS+= ${S}/arch/sun68k/sun68k/ctrlsp.S

# only needed during build
libinstall::

# bring in the required libgcc integer support:
.if defined(MACHINE_ARCH) && (${MACHINE_ARCH} == "m68000")
LIBGCC_OBJS=   _mulsi3.o _divsi3.o _udivsi3.o _modsi3.o _umodsi3.o _muldi3.o
${LIBGCC_OBJS}:
	@${AR} xv ${DESTDIR}/usr/lib/libgcc.a $@
OBJS+= ${LIBGCC_OBJS}
.endif

.include <bsd.own.mk>
.undef DESTDIR=
.include <bsd.lib.mk>

# Nuke these...
LOBJS=
POBJS=
SOBJS=

# Customized rules...

vers.c: ${.CURDIR}/version
	sh ${S}/conf/newvers_stand.sh -MD ${.CURDIR}/version "${MACHINE}"
