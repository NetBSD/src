#	$NetBSD: Makefile,v 1.2 2011/06/22 02:49:42 mrg Exp $

LIBISPRIVATE=yes

LIB=iscntp

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/lib/isc

.PATH.c::  ${DIST} ${DIST}/nls ${DIST}/unix ${DIST}/nothreads

SRCS=   assertions.c               \
        buffer.c                   \
        condition.c                \
        dir.c                      \
        error.c                    \
        errno2result.c             \
        event.c                    \
        file.c                     \
        inet_ntop.c                \
        inet_pton.c                \
        interfaceiter.c            \
        lib.c                      \
        log.c                      \
        md5.c                      \
        msgcat.c                   \
        net.c                      \
        netaddr.c                  \
        netscope.c                 \
        ondestroy.c                \
        random.c                   \
        result.c                   \
        stdio.c                    \
        stdtime.c                  \
        strerror.c                 \
        task.c                     \
        thread.c                   \
        time.c                     \
        sockaddr.c

.include <bsd.lib.mk>

# XXX
.if ${HAVE_GCC} == 45
COPTS.time.c+= -Wno-error
.endif
