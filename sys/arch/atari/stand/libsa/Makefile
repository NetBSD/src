#	$NetBSD: Makefile,v 1.27.24.1 2009/01/15 23:20:02 bouyer Exp $

LIB=	sa

NOPIC=		# defined
NOPROFILE=	# defined
NOLINT=		# defnied

NO_NET=

# Logically src/sys
S=${.CURDIR}/../../../..
S_SA=${S}/lib/libsa
S_KERN=${S}/lib/libkern
S_MACHSA=${S}/arch/atari/stand/libsa

.include "${S}/../common/lib/libc/Makefile.inc"

.include "../Makefile.booters"

CPPFLAGS+=	-I. -I${S_MACHSA} -I${S_KERN} -I${S_SA} -I${S}

.PATH:  ${S_SA} ${S_KERN}

#
# NetBSD/atari specific replacements: dev.c
#

# machine dependant routines
SRCS=	consio.S diskio.c

# from common/lib/libc
SRCS+=	ashldi3.c ashrdi3.c bzero.c strcmp.c strlen.c

# stand routines
SRCS+=	alloc.c bcmp.c bcopy.c exec.c files.c getfile.c gets.c globals.c \
	panic.c memcmp.c memcpy.c memmove.c memset.c printf.c strerror.c \
	subr_prf.c twiddle.c

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

# only needed during build
libinstall::

.include <bsd.own.mk>
.undef DESTDIR
.include <bsd.lib.mk>
