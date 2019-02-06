# $NetBSD: Makefile,v 1.18 2019/02/06 09:17:18 mrg Exp $
# Small zcat (i.e. for install media)
#

NOSSP=yes
NOMAN=
.include <bsd.own.mk>

SRCDIR=		${.CURDIR}/../../../common/dist/zlib
LIBC=		${NETBSDSRCDIR}/lib/libc

.PATH:		${SRCDIR} ${LIBC}/stdlib

WARNS?=		4
PROG=		zcat

# Just what we need from libz
SRCS=		zcat.c gzread.c gzclose.c gzlib.c inflate.c 
SRCS+=		adler32.c crc32.c zutil.c inffast.c inftrees.c
CPPFLAGS+=	-I${SRCDIR} -DNO_GZCOMPRESS

# This avoids including stdio, threads, locale, etc.
SRCS+=		misc.c
SRCS+=		malloc.c	# small
CPPFLAGS+=	-I${LIBC}/include
CPPFLAGS+=	-Dsnprintf=snprintf_ss -Dsprintf=sprintf_ss
CPPFLAGS+=	-Dstrerror=strerror_ss

.if defined(HAVE_GCC) && ${HAVE_GCC} == 7 && ${ACTIVE_CC} == "gcc"
COPTS.inflate.c+=	-Wno-error=implicit-fallthrough
.endif

.include <bsd.prog.mk>

test: zcat
	echo 'hello, hello!' | gzip | ./zcat
