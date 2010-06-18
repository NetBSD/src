#	$NetBSD: Makefile,v 1.3 2010/06/18 05:26:12 mrg Exp $

.ifdef HOSTPROG

.include <bsd.own.mk>

BINDIR=		/usr/bin

USE_FORT?= yes	# data-driven bugs?

PROG=		pigz
SRCS=		pigz.c yarn.c
MAN=		pigz.1
# these ones are from mrg's gzip
MAN+=		gzexe.1 zdiff.1 zforce.1 zgrep.1 zmore.1 znew.1

DPADD=		${LIBM} ${LIBPTHREAD}
LDADD=		-lz -lpthread

SCRIPTS=	gzexe zdiff zforce zgrep zmore znew

MLINKS+=	pigz.1 gzip.1 \
		pigz.1 gunzip.1 \
		pigz.1 gzcat.1 \
		pigz.1 zcat.1 \
		zdiff.1 zcmp.1 \
		zgrep.1 zegrep.1 \
		zgrep.1 zfgrep.1

LINKS+=		${BINDIR}/pigz ${BINDIR}/gzip \
		${BINDIR}/pigz ${BINDIR}/gunzip \
		${BINDIR}/pigz ${BINDIR}/gzcat \
		${BINDIR}/pigz ${BINDIR}/zcat \
		${BINDIR}/zdiff ${BINDIR}/zcmp \
		${BINDIR}/zgrep ${BINDIR}/zegrep \
		${BINDIR}/zgrep ${BINDIR}/zfgrep

.include <bsd.prog.mk>

DIST=		${NETBSDSRCDIR}/external/zlib/pigz/dist
.PATH:	${DIST}
GZIPDIST=	${NETBSDSRCDIR}/usr.bin/gzip
.PATH:	${GZIPDIST}

.else	# HOSTPROG

SRCS=		pigz.c

HOST_LDFLAGS=	-lz

#HAVE_PTHREAD_H!=	if echo '#include <pthread.h>' | ${HOST_CC} -E - > /dev/null; then echo yes; else echo no; fi
HAVE_PTHREAD_H?=	yes

.if ${HAVE_PTHREAD_H} != "no"
SRCS+=		yarn.c
HOST_LDFLAGS+=	-lpthread
.else
HOST_CPPFLAGS+=	-DNOTHREAD
.endif

.endif	# HOSTPROG
