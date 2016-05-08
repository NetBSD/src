#	$NetBSD: Makefile,v 1.4.18.2 2016/05/08 21:55:53 snj Exp $

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
	sha1.c			   \
        stdio.c                    \
        stdtime.c                  \
        strerror.c                 \
        task.c                     \
        thread.c                   \
        time.c                     \
	tsmemcmp.c		   \
        sockaddr.c

.if ${HAVE_GCC:U} != 4
COPTS.log.c+=	-Wno-error=format-nonliteral
.endif

.include <bsd.lib.mk>
