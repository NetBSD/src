# $NetBSD: Makefile,v 1.21.2.1 2026/11/04 22:37:11 perseant Exp $
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
CPPFLAGS+=	-I${SRCDIR} -DNO_GZCOMPRESS -DZLIB_CONST

# This avoids including stdio, threads, locale, etc.
SRCS+=		misc.c
SRCS+=		malloc.c	# small
CPPFLAGS+=	-I${LIBC}/include
CPPFLAGS+=	-Dsnprintf=snprintf_ss -Dsprintf=sprintf_ss
CPPFLAGS+=	-Dstrerror=strerror_ss

COPTS.inflate.c+=	${CC_WNO_IMPLICIT_FALLTHROUGH}

.include <bsd.prog.mk>

test: zcat
	echo 'hello, hello!' | gzip | ./zcat
