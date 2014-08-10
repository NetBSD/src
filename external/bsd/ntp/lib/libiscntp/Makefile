#	$NetBSD: Makefile,v 1.8.2.1 2014/08/10 07:09:33 tls Exp $

LIBISPRIVATE=yes

LIB=iscntp

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/lib/isc

.PATH.c::  ${DIST} ${DIST}/nls ${DIST}/unix ${DIST}/nothreads

SRCS=   assertions.c               \
	backtrace.c		   \
	backtrace-emptytbl.c	   \
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

COPTS.log.c+=	-Wno-error=format-nonliteral

.include <bsd.lib.mk>
