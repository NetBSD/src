#	$NetBSD: Makefile,v 1.4 2001/09/29 03:44:04 minoura Exp $

S=		${.CURDIR}/../../../..
LIBSADIR=	$S/lib/libsa
LIBKERNDIR=	$S/lib/libkern
LIBZDIR=	$S/lib/libz

LIB=		sa

SRCS+=		alloc.c bcmp.c bcopy.c bzero.c errno.c getfile.c gets.c
SRCS+=		memcmp.c memcpy.c memset.c panic.c
SRCS+=		printf.c snprintf.c sprintf.c subr_prf.c twiddle.c vsprintf.c

SRCS+=		close.c closeall.c cread.c dev.c fstat.c ioctl.c lseek.c
SRCS+=		open.c read.c stat.c
SRCS+=		loadfile.c
SRCS+=		cd9660.c lfs.c nfs.c ufs.c ustarfs.c
#SRCS+=		arp.c ether.c in_cksum.c net.c netif.c rpc.c udp.c
#SRCS+=		bootp.c

SRCS+=		ashldi3.c ashrdi3.c strcat.c strchr.c strcmp.c strncmp.c strcpy.c strerror.c strlen.c

SRCS+=		adler32.c crc32.c infblock.c infcodes.c inffast.c
SRCS+=		inflate.c inftrees.c infutil.c uncompr.c

SRCS+=		consio.c devopen.c parseutils.c sdcd.c fd.c fdsub.S chdsk.c

MKPROFILE=	no
MKPIC=		no
MKLINT=		no

CPPFLAGS+=	-I${.OBJDIR} -I${LIBSADIR} -I$S -I${LIBZDIR} -D_STANDALONE
CPPFLAGS+=	-I${.CURDIR}/../libiocs -I${.CURDIR}/../common
CPPFLAGS+=	-DHEAP_VARIABLE -D__INTERNAL_LIBSA_CREAD
CPPFLAGS+=	-D_ZLIB_PRIVATE
CPPFLAGS+=	-DHAVE_CHANGEDISK_HOOK
#CPPFLAGS+=	-DDEBUG

.PATH:	${LIBSADIR} ${LIBKERNDIR} ${LIBZDIR}

.if !make(obj)
.BEGIN:
	@([ -h machine ] || ln -s ${S}/arch/${MACHINE}/include machine)
	@([ -h ${MACHINE_ARCH} ] || ln -s ${S}/arch/${MACHINE_ARCH}/include ${MACHINE_ARCH})
.NOPATH: machine m68k
CLEANFILES+= machine m68k
.endif

.include <bsd.lib.mk>
