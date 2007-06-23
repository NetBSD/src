#	$NetBSD: Makefile,v 1.20 2007/06/23 10:50:25 isaki Exp $

S=		${.CURDIR}/../../../..
LIBSADIR=	$S/lib/libsa
LIBKERNDIR=	$S/lib/libkern
LIBZDIR=	$S/lib/libz
LIBZDIST=	$S/../common/dist/zlib

LIB=		sa

SRCS+=		alloc.c bcmp.c bcopy.c bzero.c errno.c getfile.c gets.c
SRCS+=		memcmp.c memcpy.c memmove.c memset.c panic.c
SRCS+=		printf.c snprintf.c sprintf.c subr_prf.c twiddle.c vsprintf.c

SRCS+=		close.c closeall.c cread.c dev.c fstat.c ioctl.c lseek.c
SRCS+=		open.c read.c stat.c
SRCS+=		loadfile.c loadfile_aout.c loadfile_elf32.c
SRCS+=		cd9660.c lfsv1.c lfsv2.c nfs.c ufs.c ufs_ls.c ustarfs.c
#SRCS+=		arp.c ether.c in_cksum.c net.c netif.c rpc.c udp.c
#SRCS+=		bootp.c

SRCS+=		ashldi3.c ashrdi3.c divdi3.c moddi3.c qdivrem.c
SRCS+=		strcat.c strchr.c strcmp.c strcpy.c strerror.c strlen.c
SRCS+=		strncmp.c strrchr.c

SRCS+=		adler32.c crc32.c inffast.c
SRCS+=		inflate.c inftrees.c uncompr.c

SRCS+=		consio.c devopen.c parseutils.c sdcd.c fd.c fdsub.S chdsk.c
SRCS+=		putimage.S

NOPROFILE=	# defined
NOPIC=		# defined
NOLINT=		# defined

CPPFLAGS+=	-I${.OBJDIR} -I${LIBSADIR} -I$S -I${LIBZDIR} -D_STANDALONE
CPPFLAGS+=	-I${.CURDIR}/../libiocs -I${.CURDIR}/../common
CPPFLAGS+=	-DHEAP_VARIABLE -D__INTERNAL_LIBSA_CREAD
CPPFLAGS+=	-DHAVE_CHANGEDISK_HOOK
#CPPFLAGS+=	-DDEBUG

.PATH:	${LIBSADIR} ${LIBKERNDIR} ${LIBZDIR} ${LIBZDIST}

.include "../Makefile.booters"
.include "${S}/../common/lib/libc/Makefile.inc"

# only needed during build
libinstall::

.include <bsd.own.mk>
.undef DESTDIR
.include <bsd.lib.mk>
