#	$NetBSD: Makefile,v 1.2 2024/10/30 18:09:18 christos Exp $

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
SYMLINKS+=      ${BINDIR}/zstd ${BINDIR}/unzstd

SCRIPTSDIR=	/usr/bin
SCRIPTS=	zstdgrep zstdless

MAN+=	zstdgrep.1 zstdless.1
MLINKS+=  zstd.1 zstdmt.1 zstd.1 zstdcat.1 zstd.1 unzstd.1

.include <bsd.prog.mk>
