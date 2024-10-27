#	$NetBSD: Makefile,v 1.1 2024/10/27 22:48:51 christos Exp $

.include <bsd.own.mk>

DIST=${.CURDIR}/../dist/
.PATH:  ${DIST}/programs

BINDIR=/usr/bin
PROG=zstd
SRCS= \
benchfn.c  \
benchzstd.c  \
datagen.c  \
dibio.c  \
fileio.c  \
fileio_asyncio.c  \
lorem.c  \
timefn.c  \
util.c  \
zstdcli.c  \
zstdcli_trace.c 

CFLAGS+=-pthread
LDFLAGS+=-pthread

PROGDPLIBS= zstd ${.CURDIR}/../lib
SYMLINKS+=      ${BINDIR}/zstd ${BINDIR}/zstdcat
SYMLINKS+=      ${BINDIR}/zstd ${BINDIR}/zstdmt

SCRIPTSDIR=	/usr/bin
SCRIPTS=	zstdgrep zstdless

.include <bsd.prog.mk>
