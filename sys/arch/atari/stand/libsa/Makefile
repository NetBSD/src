#	$NetBSD: Makefile,v 1.10 2001/10/02 06:30:03 chs Exp $

LIB=	sa

MKPIC=no
MKPROFILE=no
OBJMACHINE=

CPPFLAGS+=	${DEFS} ${INCL}
CFLAGS+=	-fomit-frame-pointer -Wall -fno-function-cse -fstrength-reduce
NO_NET=

DEFS=	-D_STANDALONE
INCL=	-I- -I${S_MACHSA} -I${S_KERN} -I${S_SA} -I${S}

#
# NetBSD/Atari specific replacements: stand.h dev.c
#

# machine dependant routines
SRCS=	consio.S diskio.c

# from lib/libkern
SRCS+=	ashldi3.c ashrdi3.c bzero.c strcmp.c strlen.c

# stand routines
SRCS+=	alloc.c bcopy.c exec.c getfile.c gets.c globals.c \
	memcpy.c printf.c strerror.c subr_prf.c twiddle.c

# io routines
SRCS+=	close.c closeall.c dev.c disklabel.c dkcksum.c ioctl.c \
	lseek.c open.c nullfs.c read.c stat.c fstat.c write.c

.if !defined(NO_NET)
# network routines
SRCS+=	arp.c ether.c in_cksum.c net.c netif.c rpc.c
.endif

# network info services:
SRCS+=	bootp.c rarp.c bootparam.c

# boot filesystems
SRCS+=	ufs.c nfs.c

# Logically src/sys
S=${.CURDIR}/../../../..
S_SA=${S}/lib/libsa
S_KERN=${S}/lib/libkern
S_MACHSA=${S}/arch/atari/stand/libsa

.PATH:  ${S_SA} ${S_KERN}

# only needed during build
libinstall::

.include <bsd.lib.mk>
