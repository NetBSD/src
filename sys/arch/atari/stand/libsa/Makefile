#	$NetBSD: Makefile,v 1.15 2002/04/08 17:25:54 bjh21 Exp $

LIB=	sa

NOPIC=# defined
NOPROFILE=# defined
OBJMACHINE=

CPPFLAGS+=	${DEFS} ${INCL}
CFLAGS+=	-fomit-frame-pointer -Wall -fno-function-cse -fstrength-reduce
NO_NET=

DEFS=	-D_STANDALONE -DHEAP_VARIABLE
INCL=	-I${S_MACHSA} -I${S_KERN} -I${S_SA} -I${S} 

#
# NetBSD/Atari specific replacements: stand.h dev.c
#

# machine dependant routines
SRCS=	consio.S diskio.c

# from lib/libkern
SRCS+=	ashldi3.c ashrdi3.c bzero.c strcmp.c strlen.c

# stand routines
SRCS+=	alloc.c bcopy.c exec.c files.c getfile.c gets.c globals.c panic.c \
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

.if !make(obj)
.BEGIN:
	@([ -h machine ] || ln -s ${S}/arch/${MACHINE}/include machine)
	@([ -h ${MACHINE_ARCH} ] || ln -s ${S}/arch/${MACHINE_ARCH}/include ${MACHINE_ARCH})
.NOPATH: machine m68k
CLEANFILES+= machine m68k
.endif

# only needed during build
libinstall::

.include <bsd.own.mk>
.undef DESTDIR
.include <bsd.lib.mk>
