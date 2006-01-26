#	$NetBSD: Makefile,v 1.22 2006/01/26 22:16:08 he Exp $

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
DIR_KERN=${S}/lib/libkern ${S}/lib/libkern/arch/m68k
DIR_LIBC=${S}/../common/lib/libc

.PATH:  ${DIR_SA} ${DIR_KERN} \
	${DIR_LIBC}/gen ${DIR_LIBC}/arch/m68k/gen \
	${DIR_LIBC}/inet ${DIR_LIBC}/arch/m68k/inet \
	${DIR_LIBC}/quad ${DIR_LIBC}/arch/m68k/quad \
	${DIR_LIBC}/string ${DIR_LIBC}/arch/m68k/string \
	${S}/arch/sun68k/sun68k

# DBG= -DDEBUG -DNETIF_DEBUG -DNFS_DEBUG -DRPC_DEBUG \
#  -DNET_DEBUG -DRARP_DEBUG -DETHER_DEBUG -DDEBUG_PROM

DEFS= -Dsun3 -D_STANDALONE -D__daddr_t=int32_t
INCL= -I. -I${.CURDIR} -I${S}/lib/libsa -I${S}
AFLAGS= -Wa,-mc68020 -Wa,-mc68851
CFLAGS= -mc68000 -Wa,-mc68010 -Wa,-m68851 -Os -msoft-float -fno-defer-pop -ffreestanding
CPPFLAGS= ${DEFS} ${DBG} ${INCL}
CLEANFILES= SRT0.o SRT1.o vers.c

SRC_net= net.c ether.c arp.c ip_cksum.c rarp.c

SRC_sa = alloc.c bcopy.c close.c files.c getfile.c loadfile.c loadfile_aout.c \
	loadfile_elf32.c lseek.c memcpy.c memmove.c open.c printf.c read.c \
	snprintf.c sprintf.c strerror.c subr_prf.c twiddle.c ctrlsp.S

SRC_kern= intoa.c

# bring in the required libgcc integer support:
SRC_libc_gen= mulsi3.S divsi3.S udivsi3.S modsi3.S umodsi3.S muldi3.c

SRC_libc_inet= inet_addr.c

SRC_libc_quad= ashldi3.c ashrdi3.c

SRC_libc_string= bcmp.S bzero.S memcmp.S memset.S strcmp.S strlen.S strncmp.S

SRC_here= clock.c dev_disk.c devopen.c \
	 gets.c idprom.c netif_sun.c panic.c \
	 promboot.c promcons.c promdev.c putstr.c \
	 sun2.c sun3.c sun3x.c vers.c xxboot.c

SRCS=	${SRC_net} ${SRC_sa} ${SRC_kern} \
	${SRC_libc_gen} ${SRC_libc_inet} ${SRC_libc_quad} ${SRC_libc_string} \
	${SRC_here}

# only needed during build
libinstall::


.include <bsd.own.mk>
.undef DESTDIR=
.include <bsd.lib.mk>

# Nuke these...
LOBJS=
POBJS=
SOBJS=

# Customized rules...

vers.c: ${.CURDIR}/version
	${HOST_SH} ${S}/conf/newvers_stand.sh -MD ${.CURDIR}/version "${MACHINE}"
